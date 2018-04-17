/** @file

  Messages that can be sent over RPC. Each should have a unique id.

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
#ifndef _MGMT_SIGNALS_H
#define _MGMT_SIGNALS_H 

/*
 * MgmtEvent defines.
 */
// Event flows: traffic manager -> traffic server
#define MGMT_EVENT_SYNC_KEY 10000
#define MGMT_EVENT_SHUTDOWN 10001
#define MGMT_EVENT_RESTART 10002
#define MGMT_EVENT_BOUNCE 10003
#define MGMT_EVENT_CLEAR_STATS 10004
#define MGMT_EVENT_CONFIG_FILE_UPDATE 10005
#define MGMT_EVENT_PLUGIN_CONFIG_UPDATE 10006
#define MGMT_EVENT_ROLL_LOG_FILES 10008
#define MGMT_EVENT_LIBRECORDS 10009
#define MGMT_EVENT_CONFIG_FILE_UPDATE_NO_INC_VERSION 10010
// cache storage operations - each is a distinct event.
// this is done because the code paths share nothing but boilerplate logic
// so it's easier to do this than to try to encode an opcode and yet another
// case statement.
#define MGMT_EVENT_STORAGE_DEVICE_CMD_OFFLINE 10011
#define MGMT_EVENT_LIFECYCLE_MESSAGE 10012
#define MGMT_EVENT_DRAIN 10013
#define MGMT_EVENT_HOST_STATUS_UP 10014
#define MGMT_EVENT_HOST_STATUS_DOWN 10015

/***********************************************************************
 *
 * MODULARIZATION: if you are adding new signals, please ensure to add
 *                 the corresponding signals in librecords/I_RecSignals.h
 *
 *
 ***********************************************************************/

// Signal flows: traffic server -> traffic manager
#define MGMT_SIGNAL_PID 100
#define MGMT_SIGNAL_MACHINE_UP 101 /* Data is ip addr */
#define MGMT_SIGNAL_MACHINE_DOWN 102
#define MGMT_SIGNAL_CONFIG_ERROR 103 /* Data is descriptive string */
#define MGMT_SIGNAL_SYSTEM_ERROR 104
#define MGMT_SIGNAL_LOG_SPACE_CRISIS 105
#define MGMT_SIGNAL_CONFIG_FILE_READ 106
#define MGMT_SIGNAL_CACHE_ERROR 107
#define MGMT_SIGNAL_CACHE_WARNING 108
#define MGMT_SIGNAL_LOGGING_ERROR 109
#define MGMT_SIGNAL_LOGGING_WARNING 110
// Currently unused: 111
// Currently unused: 112
// Currently unused: 113
#define MGMT_SIGNAL_PLUGIN_SET_CONFIG 114
#define MGMT_SIGNAL_LOG_FILES_ROLLED 115
#define MGMT_SIGNAL_LIBRECORDS 116
#define MGMT_SIGNAL_HTTP_CONGESTED_SERVER 120  /* Congestion control -- congested server */
#define MGMT_SIGNAL_HTTP_ALLEVIATED_SERVER 121 /* Congestion control -- alleviated server */

#define MGMT_SIGNAL_CONFIG_FILE_CHILD 122

#define MGMT_SIGNAL_SAC_SERVER_DOWN 400

#define MGMTAPI_SOCKET_NAME "mgmtapi.sock"

#endif /* _MGMT_SIGNALS_H */