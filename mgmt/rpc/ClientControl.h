/** @file

  A client connection to the rpc server. 

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

#ifndef _CLIENT_CONTROLLER_H
#define _CLIENT_CONTROLLER_H

#include "MgmtSignals.h"
#include "ts/ink_apidefs.h"
#include "ts/ink_sock.h"
#include <string>

class RPCClientController
{
public:
    RPCClientController();
    ~RPCClientController();

    void connectServer();
    int fd() { return rpc_sockfd; }
private: 
    std::string sockpath;
    int rpc_sockfd = ts::NO_FD;
};

#endif /* _CLIENT_CONTROLLER_H */
