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

/*****************************************************************************
* Filename: ServerControlMain.h
* Purpose: The main section for a server that handles requests to and from
*   remote clients. traffic_manager acts as the server and traffic_server,
*   traffic_ctl, etc are considered remote clients. 
***************************************************************************/

#ifndef SERVER_CONTROL_MAIN_H
#define SERVER_CONTROL_MAIN_H

#include "mgmtapi.h"
#include "MgmtCallback.h"
#include "CoreAPIShared.h" // for NUM_EVENTS

typedef struct {
  int fd;
  struct sockaddr *adr;
} RPC_Client;

RPC_Client *new_client();
void delete_client(RPC_Client *client);
void remove_client(RPC_Client *client, InkHashTable *table);

TSMgmtError init_mgmt_events();
void delete_mgmt_events();
void delete_event_queue(LLQ *q);

TSMgmtError register_server_callback(int msg_id, MgmtCallback func);
TSMgmtError unregister_server_callback(int msg_id, MgmtCallback func);

void apiAlarmCallback(alarm_t newAlarm, char *ip, char *desc);
void *server_ctrl_main(void *arg);

#endif /* SERVER_CONTROL_MAIN_H */