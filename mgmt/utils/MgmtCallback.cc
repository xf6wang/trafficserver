/** @file
 
 The Local Manager process of the management system.
 
 @section license License
 
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "ts/ink_platform.h"
#include "ts/ink_hash_table.h"
#include "ts/ink_memory.h"
#include "MgmtCallback.h"

MgmtCallbackSystem::MgmtCallbackSystem()
{
    /* Setup the event queue and callback tables */
    mgmt_incoming_queue    = create_queue();
    mgmt_callback_table = ink_hash_table_create(InkHashTableKeyType_Word);
    ink_mutex_init(&mutex);
}

MgmtCallbackSystem::~MgmtCallbackSystem()
{
    InkHashTableEntry *entry;
    InkHashTableIteratorState iterator_state;
    
    while (!queue_is_empty(mgmt_incoming_queue)) {
        MgmtMessageHdr *mh = (MgmtMessageHdr *)dequeue(mgmt_incoming_queue);
        ats_free(mh);
    }
    ats_free(mgmt_incoming_queue);

    ink_mutex_acquire(&mutex);
    for (entry = ink_hash_table_iterator_first(mgmt_callback_table, &iterator_state); entry != nullptr;
         entry = ink_hash_table_iterator_next(mgmt_callback_table, &iterator_state)) {
        MgmtCallbackList *tmp, *cb_list = (MgmtCallbackList *)entry;
        
        for (tmp = cb_list->next; tmp; tmp = cb_list->next) {
            ats_free(cb_list);
            cb_list = tmp;
        }
        ats_free(cb_list);
    }
    ink_mutex_release(&mutex);
    ink_mutex_destroy(&mutex);
}


/*
 * registerCallback(...)
 *   Function to register callback's for various management events, such
 * as shutdown, re-init, etc. The following callbacks should be
 * registered:
 *                MGMT_EVENT_SHUTDOWN  (graceful shutdown)
 *                MGMT_EVENT_RESTART   (graceful reboot)
 *                ...
 *
 *   Returns:   -1      on error(invalid event id passed in)
 *               or     value
 */
int
MgmtCallbackSystem::registerCallback(int msg_id, MgmtCallback cb, void *opaque_cb_data)
{
    MgmtCallbackList *cb_list;
    InkHashTableValue hash_value;
    
    ink_mutex_acquire(&mutex);
    if (ink_hash_table_lookup(mgmt_callback_table, (InkHashTableKey)(intptr_t)msg_id, &hash_value) != 0) {
        cb_list = (MgmtCallbackList *)hash_value;
    } else {
        cb_list = nullptr;
    }
    
    if (cb_list) {
        MgmtCallbackList *tmp;
        
        for (tmp = cb_list; tmp->next; tmp = tmp->next) {
            ;
        }
        tmp->next              = (MgmtCallbackList *)ats_malloc(sizeof(MgmtCallbackList));
        tmp->next->func        = cb;
        tmp->next->opaque_data = opaque_cb_data;
        tmp->next->next        = nullptr;
    } else {
        cb_list              = (MgmtCallbackList *)ats_malloc(sizeof(MgmtCallbackList));
        cb_list->func        = cb;
        cb_list->opaque_data = opaque_cb_data;
        cb_list->next        = nullptr;
        ink_hash_table_insert(mgmt_callback_table, (InkHashTableKey)(intptr_t)msg_id, cb_list);
    }
    ink_mutex_release(&mutex);
    return msg_id;
}

int
MgmtCallbackSystem::removeCallback(int msg_id, MgmtCallback cb)
{
    MgmtCallbackList *cb_list;
    InkHashTableValue hash_value;

    ink_mutex_acquire(&mutex);
    if (ink_hash_table_lookup(mgmt_callback_table, (InkHashTableKey)(intptr_t)msg_id, &hash_value) != 0) {
        cb_list = (MgmtCallbackList *)hash_value;
    } else {
        ink_mutex_release(&mutex);
        return 0; // doesn't exist to begin with
    }
    
    ink_assert(cb_list != nullptr);
    MgmtCallbackList *tmp;
    
    // remove callback function from linked list
    for (tmp = cb_list; tmp->next; tmp = tmp->next) {
        if(tmp->next->func == cb) {
            if((tmp->next)->next) {
                MgmtCallbackList* tmp_next = (tmp->next)->next;
                ats_free(tmp->next);
                tmp->next = tmp_next;
            }
            else { 
                ats_free(tmp->next);
                tmp->next = nullptr;
            }
            break;
        }
    }
    ink_mutex_release(&mutex);
    return 0;
}

void
MgmtCallbackSystem::triggerCallback(Args... args) 
{
    if(ink_mutex_try_acquire(&mutex)) {
        // send out message
        executeCallback(msg_id, data_raw, data_len);

        // send out any pending messages
        while(!queue_is_empty(mgmt_incoming_queue)) {
            MgmtMessageHdr *mh = (MgmtMessageHdr *)dequeue(mgmt_incoming_queue);

            //Debug("pmgmt", "processing event id '%d' payload=%d", mh->msg_id, mh->data_len);
            if (mh->data_len > 0) {
                executeCallback(mh->msg_id, (char *)mh + sizeof(MgmtMessageHdr), mh->data_len);
            } else {
                executeCallback(mh->msg_id, nullptr, 0);
            }

            ats_free(mh);
        }
        ink_mutex_release(&mutex);
    } else { // just enqueue and move on 
        MgmtMessageHdr *mh;

        mh           = (MgmtMessageHdr *)ats_malloc(sizeof(MgmtMessageHdr) + data_len);
        mh->msg_id   = msg_id;
        mh->data_len = data_len;
        memcpy((char *)mh + sizeof(MgmtMessageHdr), data_raw, data_len);

        ink_release_assert(enqueue(mgmt_incoming_queue, mh));
    }
}

void 
MgmtCallbackSystem::executeCallback(int msg_id, char *data_raw, int data_len)
{
    InkHashTableValue hash_value;

    // shouldn't call this unless caller holds mutex
    if (ink_hash_table_lookup(mgmt_callback_table, (InkHashTableKey)(intptr_t)msg_id, &hash_value) != 0) {
        for (MgmtCallbackList *cb_list = (MgmtCallbackList *)hash_value; cb_list; cb_list = cb_list->next) {
            (*((MgmtCallback)(cb_list->func)))(cb_list->opaque_data, data_raw, data_len);
        }
    }
}

