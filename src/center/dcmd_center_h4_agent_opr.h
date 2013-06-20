#ifndef __DCMD_CENTER_H4_AGENT_OPR_H__
#define __DCMD_CENTER_H4_AGENT_OPR_H__

#include <CwxCommander.h>

#include "../cmn/dcmd_tss.h"
#include "dcmd_center_def.h"

namespace dcmd {
class DcmdCenterApp;
// 处理来自agent的操作指令方面的消息
class DcmdCenterH4AgentOpr: public CwxCmdOp{
 public:
  DcmdCenterH4AgentOpr(DcmdCenterApp* app):app_(app) {
  }
  virtual ~DcmdCenterH4AgentOpr(){
  }

 public:
  // 连接关闭后，需要清理环境
  virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
  // 处理收到消息的事件
  virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
  // 消息发送成功的事件
  virtual int onEndSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
  // 消息发送失败的事件
  virtual int onFailSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
  // 向agent发送消息
  static bool SendAgentMsg(DcmdCenterApp* app,
    string const& agent_ip,
    uint32_t msg_task_id,
    CwxMsgBlock*& msg,
    uint32_t& conn_id);

 private:
  // app对象
  DcmdCenterApp*                app_;
};
}  // dcmd
#endif 