/** @file

  A brief file description

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

/**************************************
 *
 * BaseManager.h
 *   Base Manager Class, base class for all managers.
 *
 * $Date: 2004-02-03 22:12:02 $
 *
 *
 */

#ifndef _BASE_MANAGER_H
#define _BASE_MANAGER_H

#include "ts/ink_thread.h"
#include "ts/ink_mutex.h"
#include "ts/ink_llqueue.h"
#include "ts/ink_hash_table.h"

#include "MgmtDefs.h"
#include "MgmtSignals.h"
#include "MgmtMarshall.h"

/*******************************************
 * used by LocalManager and in Proxy Main. *
 */
#define MAX_OPTION_SIZE 2048
#define MAX_PROXY_SERVER_PORTS 2048
#define MAX_ATTR_LEN 5
/*******************************************/

typedef struct _mgmt_message_hdr_type {
  int msg_id;
  int data_len;
} MgmtMessageHdr;

typedef struct _mgmt_event_callback_list {
  MgmtCallback func;
  void *opaque_data;
  struct _mgmt_event_callback_list *next;
} MgmtCallbackList;

class BaseManager
{
public:
  BaseManager();
  ~BaseManager();

  int registerMgmtCallback(int msg_id, MgmtCallback func, void *opaque_callback_data = nullptr);

  LLQ *mgmt_event_queue;
  InkHashTable *mgmt_callback_table = nullptr;

protected:
  void executeMgmtCallback(int msg_id, char *data_raw, int data_len);

private:
}; /* End class BaseManager */

#endif /* _BASE_MANAGER_H */
