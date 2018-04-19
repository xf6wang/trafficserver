/** @file

Server RPC Manager

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
#include "InkAPIInternal.h"
#include "MgmtUtils.h"
#include "ts/I_Layout.h"
#include "MgmtSocket.h"

#include "ClientControl.h"

RPCClientController::RPCClientController()
{
  connectServer();
}

RPCClientController::~RPCClientController()
{
 if (rpc_sockfd != ts::NO_FD) {
    int tmp       = rpc_sockfd;
    rpc_sockfd = ts::NO_FD;
    close_socket(tmp);
  }
}
void
RPCClientController::connectServer()
{
  std::string rundir(RecConfigReadRuntimeDir());
  std::string sockpath(Layout::relative_to(rundir, MGMTAPI_SOCKET_NAME));
  if(sockpath.size() <= 0) {
    Fatal("Invalid path '%s': %s", sockpath.c_str(), strerror(errno));
  }

  int servlen;
  struct sockaddr_un serv_addr;

  if (sockpath.length() > sizeof(serv_addr.sun_path) - 1) {
    Fatal("Unable to create socket '%s': %s", sockpath.c_str(), strerror(errno));
  }

  memset((char *)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;

  ink_strlcpy(serv_addr.sun_path, sockpath.c_str(), sizeof(serv_addr.sun_path));
#if defined(darwin) || defined(freebsd)
  servlen = sizeof(sockaddr_un);
#else
  servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
#endif

  if ((rpc_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    Fatal("Unable to create socket '%s': %s", sockpath.c_str(), strerror(errno));
  }

  if (fcntl(rpc_sockfd, F_SETFD, FD_CLOEXEC) < 0) {
    Fatal("unable to set close-on-exec flag: %s", strerror(errno));
  }

  if ((connect(rpc_sockfd, (struct sockaddr *)&serv_addr, servlen)) < 0) {
    Fatal("failed to connect management socket '%s': %s", sockpath.c_str(), strerror(errno));
  }
}


 