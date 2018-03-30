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
#include "ts/ink_platform.h"
#include "ts/ink_hash_table.h"
#include "ts/ink_memory.h"
#include "MgmtHashTable.h"

#include "MgmtDefs.h"
#include "mgmtapi.h"

template<typename KEY, typename CB>
class MgmtHashTable
{
public:
    MgmtHashTable();
    virtual ~MgmtHashTable();
    
    TSMgmtError registerCallback(KEY key, CB cb);
    TSMgmtError removeCallback(KEY key);

    CB* getCallback(KEY key);
protected:
    ink_mutex mutex;
    InkHashTable *mgmt_callback_table;
}; /* End class MgmtHashTable */


template<typename KEY, typename CB>
MgmtHashTable<KEY, CB>::MgmtHashTable()
{
    /* Setup the callback table */
    mgmt_callback_table = ink_hash_table_create(InkHashTableKeyType_Word);
    ink_mutex_init(&mutex);
}

template<typename KEY, typename CB>
MgmtHashTable<KEY, CB>::~MgmtHashTable()
{
    ink_mutex_acquire(&mutex);

    ink_hash_table_destroy(mgmt_callback_table);

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
        ink_hash_table_insert(mgmt_callback_table, (InkHashTableKey)(intptr_t)key, &cb);
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
    ink_mutex_acquire(&mutex);
    TSMgmtError ret = (ink_hash_table_delete(mgmt_callback_table, (InkHashTableKey)(intptr_t)key)) ? TS_ERR_OKAY : TS_ERR_PARAMS;
    ink_mutex_release(&mutex);

    return ret;
}

template<typename KEY, typename CB>
CB*
MgmtHashTable<KEY, CB>::getCallback(KEY key)
{
    InkHashTableValue hash_value;
    if (ink_hash_table_lookup(mgmt_callback_table, (InkHashTableKey)(intptr_t)key, &hash_value) != 0) {
        return (CB*) hash_value;
    }
    return nullptr;
}

#endif /* _MGMT_CALLBACK_SYSTEM_H */