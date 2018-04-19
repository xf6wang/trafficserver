/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with rpc_server work for additional information
  regarding copyright ownership.  The ASF licenses rpc_server file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use rpc_server file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include "mgmtapi.h"
#include "ts/I_Layout.h"
#include "MgmtSignals.h"
#include "ts/ink_platform.h"
#include "ts/ink_sock.h"
#include "MgmtUtils.h"
#include "MgmtSocket.h"
#include "MgmtMarshall.h"
#include "MgmtHashTable.h"
#include "ServerControl.h"

extern RPCServerController *rpc_server;

static RecBool disable_modification = false; 

RPCServerController::RPCServerController(mode_t rpc_mode) : mode(rpc_mode)
{
  ink_mutex_init(&cb_mutex);
  client_cons = new Clients();
}

void
RPCServerController::start()
{
  Debug("start", "[ServerControl] starting rpc server ...");
  ink_thread_create(&server_thread, serverCtrlMain, &accept_con_socket, 0, 0, nullptr);
}

void
RPCServerController::stop()
{
  Debug("stop", "[ServerControl] stopping rpc server ...");
  cleanup();
  ink_thread_kill(server_thread, SIGINT);

  ink_thread_join(server_thread);
  server_thread = ink_thread_null();

  ink_mutex_acquire(&cb_mutex);
  ink_mutex_release(&cb_mutex);
  ink_mutex_destroy(&cb_mutex);

  delete client_cons;
}

int 
RPCServerController::bindSocket()
{
  
  std::string rundir(RecConfigReadRuntimeDir());
  std::string apisock(Layout::relative_to(rundir, MGMTAPI_SOCKET_NAME));
  Debug("bindSocket", "[ServerControl] binding to socket %s with mode %d", apisock.c_str(), mode);

  accept_con_socket = bind_unix_domain_socket(apisock.c_str(), mode);
  return accept_con_socket;
}

void *
RPCServerController::serverCtrlMain(void *arg)
{
  int *res       = static_cast<int *>(arg);
  int con_socket = *res;
  int ret;

  if (con_socket == ts::NO_FD) {
    Fatal("[ServerControl] not bound to a socket. exiting ...");
  }

  fd_set selectFDs; // for select call
  int fds_ready;    // stores return value for select
  struct timeval timeout;

  // loops until TM dies; waits for and processes requests from clients
  while (true) {
    // LINUX: to prevent hard-spin of CPU,  reset timeout on each loop
    timeout.tv_sec  = TIMEOUT_SECS;
    timeout.tv_usec = 0;

    FD_ZERO(&selectFDs);

    if (con_socket >= 0) {
      FD_SET(con_socket, &selectFDs);
    }

    std::unordered_set<int> clients = rpc_server->client_cons->clients();

    // add all clients to select read set.
    for (auto const &client_fd : clients) {
      if (client_fd >= 0) {
        FD_SET(client_fd, &selectFDs);
        Debug("serverCtrlMain", "[ServerControl] adding fd %d to select set", client_fd);
      }
    }
  
    // select call - timeout is set so we can check events at regular intervals
    fds_ready = mgmt_select(FD_SETSIZE, &selectFDs, (fd_set *)nullptr, (fd_set *)nullptr, &timeout);

    // check if have any connections or requests
    while (fds_ready > 0) {
      RecGetRecordBool("proxy.config.disable_configuration_modification", &disable_modification);

      // first check for connections!
      if (con_socket >= 0 && FD_ISSET(con_socket, &selectFDs)) {
        --fds_ready;
        ret = rpc_server->acceptNewConnection(con_socket);
        if (ret < 0) { // error
          Debug("serverCtrlMain", "[ServerControl] error adding connection with code %d", ret);
        }
      }

      // some other file descriptor; for each one, service request
      if (fds_ready > 0) { // Request from remote API client
        --fds_ready;
        // determine which connection the request came from.
        for (auto const &client_fd : clients) {
          Debug("serverCtrlMain", "[ServerControl] We have a remote client request!");

          if (client_fd && FD_ISSET(client_fd, &selectFDs)) {
            ret = rpc_server->handleIncomingMsg(client_fd);
            if (ret != TS_ERR_OKAY) {
              Debug("serverCtrlMain", "[ServerControl] error sending response to (%d) with code (%d)", client_fd, ret);
              rpc_server->client_cons->remove(client_fd);
              continue;
            }
          }
        }
      } // end - Request from remote API Client
    }   // end - while (fds_ready > 0)
  }     // end - while(true)

  // if we get here something's wrong, just clean up
  Debug("serverCtrlMain", "[ServerControl] CLOSING AND SHUTTING DOWN OPERATIONS");
  close_socket(con_socket);

  ink_thread_exit(nullptr);
  return nullptr;
}

TSMgmtError
RPCServerController::handleIncomingMsg(int fd)
{
  TSMgmtError ret;
  int id = 0;
  void *req     = nullptr;
  size_t reqlen = 0;

  ret = getCallbackMsg(fd, &req, &reqlen);
  if (ret == TS_ERR_NET_READ) {
    Debug("handleIncomingMsg", "[ServerControl] error - couldn't read from socket %d . dropping message.", fd);
    goto handle_fail;
  }

  if (ret == TS_ERR_NET_EOF) {
    ink_assert(reqlen == 0); // really a eof
    if (reaper) { // invoke reaper.
      Debug("handleIncomingMsg", "[ServerControl] invoking eof handler");
      reaper();
    }
    return ret;
  }

  // got a message off the socket. invoke the callback based on the message id.
  id = parseMsgId(req, reqlen);
  if (id < 0) {
    Debug("handleIncomingMsg", "[ServerControl] error unable to parse msg. got %d", id);
    goto handle_fail;
  }

  ret = executeCallback(id, fd, req, reqlen);
  if(ret != TS_ERR_OKAY) {
    Debug("handleIncomingMsg", "[ServerControl] couldn't execute callback with id %d", id);
    goto handle_fail;
  }

  // normally, the req buffer should be freed by the callback executor 
  return TS_ERR_OKAY;

handle_fail:
  ats_free(req);
  return TS_ERR_FAIL;
}

int
RPCServerController::acceptNewConnection(int fd)
{
  struct sockaddr_in clientAddr;
  socklen_t clientLen = sizeof(clientAddr);
  int new_sockfd      = mgmt_accept(fd, (struct sockaddr *)&clientAddr, &clientLen);

  Debug("acceptNewConnection", "[ServerControl] established connection to fd %d", new_sockfd);
  return rpc_server->client_cons->insert(new_sockfd);
}

TSMgmtError
RPCServerController::getCallbackMsg(int fd, void **buf, size_t *read_len)
{
  static const MgmtMarshallType lenfield[] = {MGMT_MARSHALL_DATA};
  MgmtMarshallData msg;

  *buf      = nullptr;
  *read_len = 0;

  // pull the message off the socket.
  if (mgmt_message_read(fd, lenfield, countof(lenfield), &msg) == -1) {
    return TS_ERR_NET_READ;
  }

  // We should never receive an empty payload.
  if (msg.ptr == nullptr) {
    return TS_ERR_NET_READ;
  }

  *buf      = msg.ptr;
  *read_len = msg.len;
  Debug("getCallbackMsg", "[getCallbackMsg] read message length = %zd", msg.len);
  return TS_ERR_OKAY;
}

void
RPCServerController::registerControlCallback(int key, RPCCallback cb)
{
  callbacks.insert(std::make_pair(key, cb));
}

void
RPCServerController::registerControlEOFHandler(std::function<void()> const &cb)
{
  reaper = cb;
}

int
RPCServerController::parseMsgId(void *buf, size_t buf_len)
{
  static const MgmtMarshallType idfield[] = {MGMT_MARSHALL_INT};
  MgmtMarshallInt ret;

  if (mgmt_message_parse(buf, buf_len, idfield, countof(idfield), &ret) == -1) {
    return -1;
  }

  return static_cast<int>(ret);
}

void
RPCServerController::cleanup()
{
 
  for (auto const &client_fd : client_cons->connections) {
    if (client_fd >= 0) {
      close_socket(client_fd);
    }
  }
  
}

// Get the callback function. Do not do any cleanup (free) here!
TSMgmtError
RPCServerController::executeCallback(int key, int fd, void *buf, size_t buf_len)
{
  auto it = callbacks.find(key);
  if(it != callbacks.end()) {
    CallbackExecutor *c = new CallbackExecutor(it->second, fd, buf, buf_len);
    eventProcessor.schedule_imm(c);
  } else {
    return TS_ERR_FAIL;
  }
  
  return TS_ERR_OKAY;
}


/*  Client set   */
int
RPCServerController::Clients::insert(int fd)
{
  if(fd < 0) return fd;

  ink_mutex_acquire(&mutex);
  if (connections.find(fd) == connections.end()) {
    connections.insert(fd);
  } else {
    ink_mutex_release(&mutex);
    return -1;
  }
  ink_mutex_release(&mutex);
  return 0;
}

void
RPCServerController::Clients::remove(int fd)
{
  ink_mutex_acquire(&mutex);
  connections.erase(fd);
  ink_mutex_release(&mutex);
}

/// Need constructor and destructor for ink_mutex. 
RPCServerController::Clients::Clients()
{
  ink_mutex_init(&mutex);
}

RPCServerController::Clients::~Clients()
{
  ink_mutex_acquire(&mutex);
  ink_mutex_release(&mutex);
  ink_mutex_destroy(&mutex);
}

int
RPCServerController::CallbackExecutor::rpc_callback(int event, Event *e)
{
    // execute the callback
    cb(fd, req, reqlen);
    ats_free(req);
    delete this;
    return EVENT_DONE;
}

