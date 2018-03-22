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
 * Filename: EventControlMain.cc
 * Purpose: Handles all event requests from the user.
 * Created: 01/08/01
 * Created by: lant
 *
 ***************************************************************************/

#include "ts/ink_platform.h"
#include "ts/ink_sock.h"
#include "LocalManager.h"
#include "MgmtSocket.h"
#include "MgmtMarshall.h"
#include "MgmtUtils.h"
#include "EventControlMain.h"
#include "CoreAPI.h"
#include "NetworkUtilsLocal.h"
#include "NetworkMessage.h"

// variables that are very important
ink_mutex mgmt_events_lock;
LLQ *mgmt_events;
InkHashTable *accepted_clients; // list of all accepted client connections

static TSMgmtError handle_event_message(EventClientT *client, void *req, size_t reqlen);

/*********************************************************************
 * new_event_client
 *
 * purpose: creates a new EventClientT and return pointer to it
 * input: None
 * output: EventClientT
 * note: None
 *********************************************************************/
EventClientT *
new_event_client()
{
  EventClientT *ele = (EventClientT *)ats_malloc(sizeof(EventClientT));

  // now set the alarms registered section
  for (bool &i : ele->events_registered) {
    i = false;
  }

  ele->adr = (struct sockaddr *)ats_malloc(sizeof(struct sockaddr));
  return ele;
}

/*********************************************************************
 * delete_event_client
 *
 * purpose: frees memory allocated for an EventClientT
 * input: EventClientT
 * output: None
 * note: None
 *********************************************************************/
void
delete_event_client(EventClientT *client)
{
  if (client) {
    ats_free(client->adr);
    ats_free(client);
  }
  return;
}

/*********************************************************************
 * remove_event_client
 *
 * purpose: removes the EventClientT from the specified hashtable; includes
 *          removing the binding and freeing the ClientT
 * input: client - the ClientT to remove
 * output:
 *********************************************************************/
void
remove_event_client(EventClientT *client, InkHashTable *table)
{
  // close client socket
  close_socket(client->fd); // close client socket

  // remove client binding from hash table
  ink_hash_table_delete(table, (char *)&client->fd);

  // free ClientT
  delete_event_client(client);

  return;
}

/*********************************************************************
 * init_mgmt_events
 *
 * purpose: initializes the mgmt_events queue which is intended to hold
 *          TM events.
 * input:
 * output: TS_ERR_xx
 * note: None
 *********************************************************************/
TSMgmtError
init_mgmt_events()
{
  ink_mutex_init(&mgmt_events_lock);

  // initialize queue
  mgmt_events = create_queue();
  if (!mgmt_events) {
    ink_mutex_destroy(&mgmt_events_lock);
    return TS_ERR_SYS_CALL;
  }

  return TS_ERR_OKAY;
}

/*********************************************************************
 * delete_mgmt_events
 *
 * purpose: frees the mgmt_events queue.
 * input:
 * output: None
 * note: None
 *********************************************************************/
void
delete_mgmt_events()
{
  // obtain lock
  ink_mutex_acquire(&mgmt_events_lock);

  // delete the queue associated with the queue of events
  delete_event_queue(mgmt_events);

  // release it
  ink_mutex_release(&mgmt_events_lock);

  // kill lock
  ink_mutex_destroy(&mgmt_events_lock);

  delete_queue(mgmt_events);

  return;
}

/*********************************************************************
 * delete_event_queue
 *
 * purpose: frees queue where the elements are of type TSMgmtEvent* 's
 * input: LLQ * q - a queue with entries of TSMgmtEvent*'s
 * output: None
 * note: None
 *********************************************************************/
void
delete_event_queue(LLQ *q)
{
  if (!q) {
    return;
  }

  // now for every element, dequeue and free
  TSMgmtEvent *ele;

  while (!queue_is_empty(q)) {
    ele = (TSMgmtEvent *)dequeue(q);
    ats_free(ele);
  }

  delete_queue(q);
  return;
}

/*********************************************************************
 * apiEventCallback
 *
 * purpose: callback function registered with alarm processor so that
 *          each time alarm is signalled, can enqueue it in the mgmt_events
 *          queue
 * input:
 * output: None
 * note: None
 *********************************************************************/

void
apiEventCallback(alarm_t newAlarm, const char * /* ip ATS_UNUSED */, const char *desc)
{
  // create an TSMgmtEvent
  // addEvent(new_alarm, ip, desc) // adds event to mgmt_events
  TSMgmtEvent *newEvent;

  newEvent       = TSEventCreate();
  newEvent->id   = newAlarm;
  newEvent->name = get_event_name(newEvent->id);
  // newEvent->ip   = ats_strdup(ip);
  if (desc) {
    newEvent->description = ats_strdup(desc);
  } else {
    newEvent->description = ats_strdup("None");
  }

  // add it to the mgmt_events list
  ink_mutex_acquire(&mgmt_events_lock);
  enqueue(mgmt_events, newEvent);
  ink_mutex_release(&mgmt_events_lock);

  return;
}


/*-------------------------------------------------------------------------
                             HANDLER FUNCTIONS
 --------------------------------------------------------------------------*/

/**************************************************************************
 * handle_event_reg_callback
 *
 * purpose: handles request to register a callback for a specific event (or all events)
 * input: client - the client currently reading the msg from
 *        req    - the event_name
 * output: TS_ERR_xx
 * note: the req should be the event name; does not send a reply to client
 *************************************************************************/
static TSMgmtError
handle_event_reg_callback(EventClientT *client, void *req, size_t reqlen)
{
  MgmtMarshallInt optype;
  MgmtMarshallString name = nullptr;
  TSMgmtError ret;

  ret = recv_mgmt_request(req, reqlen, OpType::EVENT_REG_CALLBACK, &optype, &name);
  if (ret != TS_ERR_OKAY) {
    goto done;
  }

  // mark the specified alarm as "wanting to be notified" in the client's alarm_registered list
  if (strlen(name) == 0) { // mark all alarms
    for (bool &i : client->events_registered) {
      i = true;
    }
  } else {
    int id = get_event_id(name);
    if (id < 0) {
      ret = TS_ERR_FAIL;
      goto done;
    }

    client->events_registered[id] = true;
  }

  ret = TS_ERR_OKAY;

done:
  ats_free(name);
  return ret;
}

/**************************************************************************
 * handle_event_unreg_callback
 *
 * purpose: handles request to unregister a callback for a specific event (or all events)
 * input: client - the client currently reading the msg from
 *        req    - the event_name
 * output: TS_ERR_xx
 * note: the req should be the event name; does not send reply to client
 *************************************************************************/
static TSMgmtError
handle_event_unreg_callback(EventClientT *client, void *req, size_t reqlen)
{
  MgmtMarshallInt optype;
  MgmtMarshallString name = nullptr;
  TSMgmtError ret;

  ret = recv_mgmt_request(req, reqlen, OpType::EVENT_UNREG_CALLBACK, &optype, &name);
  if (ret != TS_ERR_OKAY) {
    goto done;
  }

  // mark the specified alarm as "wanting to be notified" in the client's alarm_registered list
  if (strlen(name) == 0) { // mark all alarms
    for (bool &i : client->events_registered) {
      i = false;
    }
  } else {
    int id = get_event_id(name);
    if (id < 0) {
      ret = TS_ERR_FAIL;
      goto done;
    }

    client->events_registered[id] = false;
  }

  ret = TS_ERR_OKAY;

done:
  ats_free(name);
  return ret;
}

using event_message_handler = TSMgmtError (*)(EventClientT *, void *, size_t);

static const event_message_handler handlers[] = {
  nullptr,                     // RECORD_SET
  nullptr,                     // RECORD_GET
  nullptr,                     // PROXY_STATE_GET
  nullptr,                     // PROXY_STATE_SET
  nullptr,                     // RECONFIGURE
  nullptr,                     // RESTART
  nullptr,                     // BOUNCE
  nullptr,                     // EVENT_RESOLVE
  nullptr,                     // EVENT_GET_MLT
  nullptr,                     // EVENT_ACTIVE
  handle_event_reg_callback,   // EVENT_REG_CALLBACK
  handle_event_unreg_callback, // EVENT_UNREG_CALLBACK
  nullptr,                     // EVENT_NOTIFY
  nullptr,                     // DIAGS
  nullptr,                     // STATS_RESET_NODE
  nullptr,                     // STORAGE_DEVICE_CMD_OFFLINE
  nullptr,                     // RECORD_MATCH_GET
  nullptr,                     // LIFECYCLE_MESSAGE
};

static TSMgmtError
handle_event_message(EventClientT *client, void *req, size_t reqlen)
{
  OpType optype = extract_mgmt_request_optype(req, reqlen);

  if (static_cast<unsigned>(optype) >= countof(handlers)) {
    goto fail;
  }

  if (handlers[static_cast<unsigned>(optype)] == nullptr) {
    goto fail;
  }

  if (mgmt_has_peereid()) {
    uid_t euid = -1;
    gid_t egid = -1;

    // For now, all event messages require privilege. This is compatible with earlier
    // versions of Traffic Server that always required privilege.
    if (mgmt_get_peereid(client->fd, &euid, &egid) == -1 || (euid != 0 && euid != geteuid())) {
      return TS_ERR_PERMISSION_DENIED;
    }
  }

  return handlers[static_cast<unsigned>(optype)](client, req, reqlen);

fail:
  mgmt_elog(0, "%s: missing handler for type %d event message\n", __func__, (int)optype);
  return TS_ERR_PARAMS;
}
