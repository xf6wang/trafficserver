/** @file

  Network message marshalling.

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

#include "ts/ink_config.h"
#include "ts/ink_defs.h"
#include "ts/ink_assert.h"
#include "ts/ink_memory.h"
#include "mgmtapi.h"
#include "NetworkMessage.h"
#include <functional>

#define MAX_OPERATION_BUFSZ 1024
#define MAX_OPERATION_FIELDS 16

struct NetCmdOperation {
  unsigned nfields;
  const MgmtMarshallType fields[MAX_OPERATION_FIELDS];
};

struct NetCmd {
  typedef std::function<TSMgmtError(int, OpType, int)> NetMsgErrHandler;
  struct NetCmdOperation request;
  struct NetCmdOperation response;
  NetMsgErrHandler err_handler;
};

static TSMgmtError err_ts_ok_handler(int fd, OpType optype, MgmtMarshallInt ecode);
static TSMgmtError err_ecode_handler(int fd, OpType optype, MgmtMarshallInt ecode);
static TSMgmtError err_intval_handler(int fd, OpType optype, MgmtMarshallInt ecode);
static TSMgmtError err_strval_handler(int fd, OpType optype, MgmtMarshallInt ecode);
static TSMgmtError err_multival_handler(int fd, OpType optype, MgmtMarshallInt ecode);
static TSMgmtError err_record_desc_config_handler(int fd, OpType optype, MgmtMarshallInt ecode);

/**
 NetOpertation Structure
 { request,       // Requests always begin with a OpType, followed by aditional fields.
   response,      // Responses always begin with a TSMgmtError code, followed by additional fields.
   error handler  // 
 }
*/

static const struct NetCmd NetOperations [] = {
  /* RECORD_SET                 */  { {3, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING, MGMT_MARSHALL_STRING}},
                                      {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      err_intval_handler 
                                    },
  /* RECORD_GET                 */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {5, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT, MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING, MGMT_MARSHALL_DATA}}, 
                                      err_multival_handler
                                    },
  /* PROXY_STATE_GET            */  { {1, {MGMT_MARSHALL_INT}},
                                      {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      err_intval_handler
                                    },       
  /* PROXY_STATE_SET            */  { {3, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler
                                    },
  /* RECONFIGURE                */  { {1, {MGMT_MARSHALL_INT}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler
                                    },                                                                                                                           
  /* RESTART                    */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler
                                    },
  /* BOUNCE                     */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler
                                    },
  /* EVENT_RESOLVE              */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler
                                    },
 /* EVENT_GET_MLT              */   { {1, {MGMT_MARSHALL_INT}},
                                      {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      err_strval_handler
                                    },
 /* EVENT_ACTIVE               */   { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      err_intval_handler
                                    },
 /* EVENT_REG_CALLBACK         */   { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {0, {}}, // no reply
                                      err_ts_ok_handler
                                    },                         
  /* EVENT_UNREG_CALLBACK       */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {0, {}}, // no reply
                                      err_ts_ok_handler
                                    },
  /* EVENT_NOTIFY               */  { {3, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING, MGMT_MARSHALL_STRING}}, // only msg sent from TM to client
                                      {0, {}}, // no reply
                                      err_ts_ok_handler
                                    },              
  /* STATS_RESET_NODE           */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler
                                    },   
  /* STORAGE_DEVICE_CMD_OFFLINE */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler                              
                                    },
  /* RECORD_MATCH_GET           */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      {5, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT, MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING, MGMT_MARSHALL_DATA}},
                                      err_multival_handler
                                    },
  /* API_PING                   */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      {0, {}}, // no reply
                                      err_ts_ok_handler
                                    },         
  /* SERVER_BACKTRACE           */  { {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_INT}},
                                      {2, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING}},
                                      err_strval_handler
                                    },
  /* RECORD_DESCRIBE_CONFIG     */  { {3, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING, MGMT_MARSHALL_INT}},
                                      {15,
                                        {MGMT_MARSHALL_INT /* status */, MGMT_MARSHALL_STRING /* name */, MGMT_MARSHALL_DATA /* value */,
                                          MGMT_MARSHALL_DATA /* default */, MGMT_MARSHALL_INT /* type */, MGMT_MARSHALL_INT /* class */, MGMT_MARSHALL_INT /* version */,
                                          MGMT_MARSHALL_INT /* rsb */, MGMT_MARSHALL_INT /* order */, MGMT_MARSHALL_INT /* access */, MGMT_MARSHALL_INT /* update */,
                                          MGMT_MARSHALL_INT /* updatetype */, MGMT_MARSHALL_INT /* checktype */, MGMT_MARSHALL_INT /* source */,
                                          MGMT_MARSHALL_STRING /* checkexpr */}},
                                      err_record_desc_config_handler
                                    },
  /* LIFECYCLE_MESSAGE          */  { {3, {MGMT_MARSHALL_INT, MGMT_MARSHALL_STRING, MGMT_MARSHALL_DATA}},
                                      {1, {MGMT_MARSHALL_INT}},
                                      err_ecode_handler
                                    }                                                                                                                                                                                                  
};

#define GETCMD(ops, optype, cmd)                           \
  do {                                                     \
    if (static_cast<unsigned>(optype) >= countof(ops)) {   \
      return TS_ERR_PARAMS;                                \
    }                                                      \
    cmd = &ops[static_cast<unsigned>(optype)];             \
  } while (0);

TSMgmtError
send_mgmt_error(int fd, OpType optype, TSMgmtError error)
{
  if(optype == OpType::UNDEFINED_OP) { // have to handle seperately because we don't know know the field length
    return TS_ERR_OKAY;
  }
  const NetCmd *cmd;
  GETCMD(NetOperations, optype, cmd);

  const std::function<TSMgmtError(int, OpType, int)>* handler = &(cmd->err_handler);
  return (*handler)(fd, optype, error); 
}

TSMgmtError
send_mgmt_message(MgmtMessageType msgtype, int fd, OpType optype, ...)
{
  va_list ap;
  MgmtMarshallInt msglen;
  MgmtMarshallData req            = {nullptr, 0};
  const MgmtMarshallType fields[] = {MGMT_MARSHALL_DATA};
  const NetCmd *cmd;
  const NetCmdOperation *operation;

  GETCMD(NetOperations, optype, cmd);

  switch(msgtype){
  case MgmtMessageType::REQUEST:
    operation = &(cmd->request);
    break;
  case MgmtMessageType::RESPONSE:
    operation = &(cmd->response);
    break;
  case MgmtMessageType::ERROR:
    ink_fatal("invalid message type. should use send_mgmt_error, dropping message: %d", static_cast<int>(msgtype));
  default:
    ink_fatal("invalid message type. %d", static_cast<int>(msgtype));
    return TS_ERR_PARAMS;
  }

  // Figure out the payload length.
  va_start(ap, optype);
  msglen = mgmt_message_length_v(operation->fields, operation->nfields, ap);
  va_end(ap);

  ink_assert(msglen >= 0);

  req.ptr = (char *)ats_malloc(msglen);
  req.len = msglen;

  // Marshall the message itself.
  va_start(ap, optype);
  if (mgmt_message_marshall_v(req.ptr, req.len, operation->fields, operation->nfields, ap) == -1) {
    ats_free(req.ptr);
    va_end(ap);
    return TS_ERR_PARAMS;
  }

  va_end(ap);

  // Send the response as the payload of a data object.
  if (mgmt_message_write(fd, fields, countof(fields), &req) == -1) {
    ats_free(req.ptr);
    return TS_ERR_NET_WRITE;
  }

  ats_free(req.ptr);
  return TS_ERR_OKAY;
}

TSMgmtError
send_mgmt_request(const mgmt_message_sender &snd, OpType optype, ...)
{
  va_list ap;
  ats_scoped_mem<char> msgbuf;
  MgmtMarshallInt msglen;
  const MgmtMarshallType lenfield[] = {MGMT_MARSHALL_INT};
  const NetCmd *cmd;
  const NetCmdOperation *operation;

  if (!snd.is_connected())
    return TS_ERR_NET_ESTABLISH; // no connection.

  GETCMD(NetOperations, optype, cmd);
  operation = &(cmd->request);

  va_start(ap, optype);
  msglen = mgmt_message_length_v(operation->fields, operation->nfields, ap);
  va_end(ap);

  msgbuf = (char *)ats_malloc(msglen + 4);

  // First marshall the total message length.
  mgmt_message_marshall((char *)msgbuf, msglen, lenfield, countof(lenfield), &msglen);

  // Now marshall the message itself.
  va_start(ap, optype);
  if (mgmt_message_marshall_v((char *)msgbuf + 4, msglen, operation->fields, operation->nfields, ap) == -1) {
    va_end(ap);
    return TS_ERR_PARAMS;
  }

  va_end(ap);
  return snd.send(msgbuf, msglen + 4);
}

TSMgmtError
recv_x(MgmtMessageType msgtype, void *buf, size_t buflen, OpType optype, va_list ap)
{
  ssize_t msglen;
  const NetCmd *cmd;
  const NetCmdOperation *operation = nullptr;
  
  GETCMD(NetOperations, optype, cmd);

  switch(msgtype){
  case REQUEST:
    operation = &(cmd->request);
    break;
  case RESPONSE:
    operation = &(cmd->response);
    break;
  case MgmtMessageType::ERROR: // should never recieve an error msg. should just be a response
  default:
    ink_fatal("invalid message type. %d", static_cast<int>(msgtype));
    return TS_ERR_PARAMS;
  }
  
  ink_assert(operation != nullptr);

  msglen = mgmt_message_parse_v(buf, buflen, operation->fields, operation->nfields, ap);
  return (msglen == -1) ? TS_ERR_PARAMS : TS_ERR_OKAY;
}

TSMgmtError 
recv_mgmt_message(MgmtMessageType msgtype, void *buf, size_t buflen, OpType optype, ...)
{
  TSMgmtError err;
  va_list ap;
 
  va_start(ap, optype);
  err = recv_x(msgtype, buf, buflen, optype, ap);
  va_end(ap);

  return err;
}

TSMgmtError
recv_mgmt_message(int fd, MgmtMarshallData &msg)
{
  const MgmtMarshallType fields[] = {MGMT_MARSHALL_DATA};

  if (mgmt_message_read(fd, fields, countof(fields), &msg) == -1) {
    return TS_ERR_NET_READ;
  }

  return TS_ERR_OKAY;
}

OpType
extract_mgmt_request_optype(void *msg, size_t msglen)
{
  const MgmtMarshallType fields[] = {MGMT_MARSHALL_INT};
  MgmtMarshallInt optype;

  if (mgmt_message_parse(msg, msglen, fields, countof(fields), &optype) == -1) {
    return OpType::UNDEFINED_OP;
  }

  return (OpType)optype;
}

//------------- DEFINE HANDLER FUNCTIONS ------------------

static TSMgmtError err_ts_ok_handler(int fd, OpType optype, MgmtMarshallInt ecode) {
  return TS_ERR_OKAY;
}
static TSMgmtError err_ecode_handler(int fd, OpType optype, MgmtMarshallInt ecode) {
  return send_mgmt_message(RESPONSE, fd, optype, &ecode);
}
static TSMgmtError err_intval_handler(int fd, OpType optype, MgmtMarshallInt ecode) {
  MgmtMarshallInt intval    = 0;
  return send_mgmt_message(RESPONSE, fd, optype, &ecode, &intval);
}
static TSMgmtError err_strval_handler(int fd, OpType optype, MgmtMarshallInt ecode) {
  MgmtMarshallString strval = nullptr;
  return send_mgmt_message(RESPONSE, fd, optype, &ecode, &strval);
}
static TSMgmtError err_multival_handler(int fd, OpType optype, MgmtMarshallInt ecode) {
  MgmtMarshallInt intval    = 0;
  MgmtMarshallData dataval  = {nullptr, 0};
  MgmtMarshallString strval = nullptr;
  return send_mgmt_message(RESPONSE, fd, optype, &ecode, &intval, &intval, &strval, &dataval);
}
static TSMgmtError err_record_desc_config_handler(int fd, OpType optype, MgmtMarshallInt ecode) {
  MgmtMarshallInt intval    = 0;
  MgmtMarshallData dataval  = {nullptr, 0};
  MgmtMarshallString strval = nullptr;
  return send_mgmt_message(RESPONSE, fd, optype, &ecode, &strval /* name */, &dataval /* value */, &dataval /* default */,
                                            &intval /* type */, &intval /* class */, &intval /* version */, &intval /* rsb */,
                                            &intval /* order */, &intval /* access */, &intval /* update */, &intval /* updatetype */,
                                            &intval /* checktype */, &intval /* source */, &strval /* checkexpr */);
}


