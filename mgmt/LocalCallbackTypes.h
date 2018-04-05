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
#ifndef _MGMT_CALLBACK_TYPES_H
#define _MGMT_CALLBACK_TYPES_H

/* SHOULD EXACTLY MATCH CallbackTypes.h */

struct ControlHandler {
  unsigned flags;
  TSMgmtError (*handler)(int, void *, size_t);
};

#define MGMT_CONTROL_FUNCTION 0
#define MGMT_LM_CALLBACK 1

#endif /* _MGMT_CALLBACK_TYPES_H */
