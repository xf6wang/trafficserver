/** @file

  A rpc server. 

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

#ifndef _SERVER_CONTROLLER_H
#define _SERVER_CONTROLLER_H

#include "mgmtapi.h"
#include "I_EventSystem.h"

#include "ts/ink_mutex.h"

#include <functional>
#include <unordered_set>
#include <unordered_map>

template <typename... Params>
class RPCCallbackBase 
{
using Parameters = std::tuple<Params ...>;
public:
  RPCCallbackBase() : params(nullptr) {}
  explicit RPCCallbackBase(Parameters p) : params(p) {}

  Parameters get() {return params;}
  void set(Parameters p) {params = p;}

  virtual TSMgmtError execute() = 0;
private:
  Parameters params;
};

/**
  Simple RPC server. Allows for registration of callbacks in the form (int fd, void* buf, size_t buflen). Currently, it is the
  responsibility of the callback function to deseralize the incoming message.

  This class does not send out any outgoing messages, that is the responsibility of the callback function. 
  Incoming messages always arrive in the following form: int (message length), int (message id), message.
 */
class RPCServerController 
{
  /// No deserialization logic so we must force all callbacks to be of the same form. 
  // typedef std::function<TSMgmtError(int, void*, size_t)> RPCCallback;
  typedef std::function<void()> eofCallback;

  struct Clients 
  {
    std::unordered_set<int> connections; // a set of all accepted client connections
    ink_mutex mutex;

    Clients();
    ~Clients();

    std::unordered_set<int> clients() { return connections; }
    int insert(int fd);
    void remove(int fd);
  };

  struct CallbackExecutor : public Continuation {
    CallbackExecutor(RPCCallback _cb, int _fd, void *_req, size_t _reqlen) : Continuation(new_ProxyMutex()), cb(_cb), fd(_fd), req(_req), reqlen(_reqlen) { SET_HANDLER(&CallbackExecutor::rpc_callback); }

    int rpc_callback(int event, Event * /* e ATS_UNUSED */);
    RPCCallback cb;
    int fd;
    void *req;
    size_t reqlen;
  };
  
public:
  /** Creates a RPCServerController object with a @rpc_mode operating mode. 
   */
  explicit RPCServerController(mode_t rpc_mode);

  /// Add a callback based on a key. Each key must be unique in MgmtSignals.h
  void registerControlCallback(int key, RPCCallbackBase );
  void registerControlEOFHandler(eofCallback const &cb = std::function<void()>());

  int bindSocket();

  void start(); // only start after all callbacks have been registered and socket has been binded.
  void stop();

private:
  static constexpr int TIMEOUT_SECS = 1;

  Clients *client_cons;

  mode_t mode;
  int accept_con_socket = ts::NO_FD;

  eofCallback reaper = nullptr;
  std::unordered_map<int, RPCCallback> callbacks;
  ink_mutex cb_mutex;

  int parseMsgId(void *buf, size_t buf_len);

  TSMgmtError getCallbackMsg(int fd, void **buf, size_t *read_len);
  TSMgmtError executeCallback(int key, int fd, void *buf, size_t buf_len);
  TSMgmtError handleIncomingMsg(int con_fd);
  int acceptNewConnection(int con_socket);

  void cleanup();

  /// main executing thread. one for each RPC controller. This is necessary because
  /// traffic_server also acts as a rpc server so it must also have a thread to accept incoming messages
  ink_thread server_thread = ink_thread_null();
  static void *serverCtrlMain(void *arg);
};
#endif /* _SERVER_CONTROLLER_H */