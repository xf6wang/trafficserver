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
 * BaseManager.cc
 *   Member function definitions for Base Manager class.
 *
 * $Date: 2003-06-01 18:37:18 $
 *
 *
 */

#include "ts/ink_platform.h"
#include "ts/ink_hash_table.h"
#include "ts/ink_memory.h"
#include "BaseManager.h"

BaseManager::BaseManager()
{
  mgmt_event_queue    = create_queue();
  mgmt_callback_handler = new LocalMgmtHashTable();
} /* End BaseManager::BaseManager */

BaseManager::~BaseManager()
{
  while (!queue_is_empty(mgmt_event_queue)) {
      MgmtMessageHdr *mh = (MgmtMessageHdr *)dequeue(mgmt_event_queue);
      ats_free(mh);
  }
  ats_free(mgmt_event_queue);

  delete mgmt_callback_handler;
} /* End BaseManager::~BaseManager */

int 
BaseManager::registerMgmtCallback(int msg_id, MgmtCallback func, void *opaque_callback_data)
{
  return mgmt_callback_handler->registerCallback(msg_id, func, opaque_callback_data);
}

void 
BaseManager::sendMgmtEvent(int msg_id, char* data_raw, int data_len)
{
  mgmt_callback_handler->triggerCallback(msg_id, data_raw, data_len);
}