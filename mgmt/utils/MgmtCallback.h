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

#ifndef _MGMT_CALLBACK_SYSTEM_H
#define _MGMT_CALLBACK_SYSTEM_H

#include "ts/ink_thread.h"
#include "ts/ink_mutex.h"
#include "ts/ink_llqueue.h"
#include "ts/ink_hash_table.h"

#include "MgmtDefs.h"
#include "MgmtMarshall.h"

typedef struct _mgmt_message_hdr_type {
    int msg_id;
    int data_len;
} MgmtMessageHdr;

typedef struct _mgmt_event_callback_list {
    MgmtCallback func;
    void *opaque_data;
    struct _mgmt_event_callback_list *next;
} MgmtCallbackList;

class MgmtCallbackSystem
{
public:
    MgmtCallbackSystem();
    ~MgmtCallbackSystem();
    
    int registerCallback(int msg_id, MgmtCallback func, void *opaque_callback_data = NULL);
    int removeCallback(int msg_id, MgmtCallback func);
    
    void triggerCallback(int msg_id, char *data_raw, int data_len);
protected:
    ink_mutex mutex;
    LLQ *mgmt_incoming_queue;
    InkHashTable *mgmt_callback_table;

private:
    void executeCallback(int msg_id, char *data_raw, int data_len);
    
}; /* End class MgmtCallbackSystem */

#endif /* _MGMT_CALLBACK_SYSTEM_H */
