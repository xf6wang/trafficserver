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
#include "MgmtHashTable.h"

template<typename KEY, typename CB>
MgmtHashTable<KEY, CB>::MgmtHashTable()
{
    /* Setup the event queue and callback tables */
    mgmt_incoming_queue    = create_queue();
    mgmt_callback_table = ink_hash_table_create(InkHashTableKeyType_Word);
    ink_mutex_init(&mutex);
}

template<typename KEY, typename CB>
MgmtHashTable<KEY, CB>::~MgmtHashTable()
{
    InkHashTableEntry *entry;
    InkHashTableIteratorState iterator_state;
    
    while (!queue_is_empty(mgmt_incoming_queue)) {
        //MgmtMessageHdr *mh = (MgmtMessageHdr *)dequeue(mgmt_incoming_queue);
        //ats_free(mh);
    }
    ats_free(mgmt_incoming_queue);

    ink_mutex_acquire(&mutex);
    // for (entry = ink_hash_table_iterator_first(mgmt_callback_table, &iterator_state); entry != nullptr;
    //      entry = ink_hash_table_iterator_next(mgmt_callback_table, &iterator_state)) {
    //     MgmtCallbackList *tmp, *cb_list = (MgmtCallbackList *)entry;
        
    //     for (tmp = cb_list->next; tmp; tmp = cb_list->next) {
    //         ats_free(cb_list);
    //         cb_list = tmp;
    //     }
    //     ats_free(cb_list);
    // }
    ink_mutex_release(&mutex);
    ink_mutex_destroy(&mutex);
}


template<typename KEY, typename CB>
TSMgmtError
MgmtHashTable<KEY, CB>::registerCallback(KEY key, CB cb)
{
    InkHashTableValue hash_value;
    TSMgmtError ret = TS_ERR_OKAY;

    ink_mutex_acquire(&mutex);
    if (ink_hash_table_lookup(mgmt_callback_table, (InkHashTableKey)(intptr_t)key, &hash_value) == 0) {
        ink_hash_table_insert(mgmt_callback_table, (InkHashTableKey)(intptr_t)key, cb);
    } else {
        ret = TS_ERR_PARAMS; // fatal 
    }
    
    ink_mutex_release(&mutex);
    return ret;
}

template<typename KEY, typename CB>
TSMgmtError
MgmtHashTable<KEY, CB>::removeCallback(KEY key)
{
    InkHashTableValue hash_value;

    ink_mutex_acquire(&mutex);
    TSMgmtError ret = (ink_hash_table_delete(mgmt_callback_table, (InkHashTableKey)(intptr_t)key)) ? TS_ERR_OKAY : TS_ERR_PARAMS;
    ink_mutex_release(&mutex);

    return ret;
}