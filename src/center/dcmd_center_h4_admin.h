﻿#ifndef __DCMD_CENTER_H_4_ADMIN_H__
#define __DCMD_CENTER_H_4_ADMIN_H__

#include <CwxCommander.h>

#include "../cmn/dcmd_tss.h"
#include "dcmd_center_def.h"

namespace dcmd {

class DcmdCenterApp;
// 处理来自控制台的消息
class DcmdCenterH4Admin: public CwxCmdOp {
 public:
  DcmdCenterH4Admin(DcmdCenterApp* app):app_(app) {
    next_msg_task_id_ = 1;
  }
  virtual ~DcmdCenterH4Admin(){
  }

 public:
  // 处理收到来自控制台的事件
  virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
  // 检测cmd执行超时
  virtual int onTimeoutCheck(CwxMsgBlock*& msg, CwxTss* pThrEnv);
 public:
  // 回复操作指令
  static void ReplyExecOprCmd(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiExecOprCmdReply* result);
  // 回复agent的subtask的output查询
  static void ReplySubTaskOutput(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiTaskOutputReply* result);
  // 回复agent运行subtask查询
  static void ReplyAgentRunSubTask(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiAgentRunningTaskReply* result);
  // 回复agent运行opr操作查询
  static void ReplyAgentRunOprCmd(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiAgentRunningOprReply* result);
  // 回复agent状态查询
  static void ReplyAgentStatus(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiAgentInfoReply* result);
  // 回复非法agent查询
  static void ReplyIllegalAgent(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiInvalidAgentInfoReply* result);
  // 回复查询操作指令script内容
  static void ReplyOprScriptContent(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiOprScriptInfoReply* result);
  // 回复查询task-cmd的script的内容
  static void ReplyTaskCmdScriptContent(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiTaskScriptInfoReply* result);
  // 回复任务执行进度查询
  static void ReplyAgentSubTaskProcess(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t conn_id,
    uint32_t msg_task_id,
    dcmd_api::UiAgentTaskProcessReply* result);
  // 回复命令的执行结果
  static void ReplyUiTaskCmd(DcmdCenterApp* app,
    DcmdTss* tss,
    uint32_t  conn_id,
    uint32_t  msg_task_id,
    dcmd_api::UiTaskCmdReply* result);

 private:
  // 执行操作指令
  void ExecOprCmd(CwxMsgBlock*& msg, DcmdTss* tss);
  // 查询agent的subtask的输出
  void QuerySubtaskOutput(CwxMsgBlock*& msg, DcmdTss* tss);
  // 查询agent上运行的subtask的信息
  void QueryAgentRunSubTask(CwxMsgBlock*& msg, DcmdTss* tss);
  // 查询Agent上运行opr的信息
  void QueryAgentRunOpr(CwxMsgBlock*& msg, DcmdTss* tss);
  // 查询agent的状态
  void QueryAgentStatus(CwxMsgBlock*& msg, DcmdTss* tss);
  // 获取非法的agent
  void QueryIllegalAgent(CwxMsgBlock*& msg, DcmdTss* tss);
  // 获取opr script的content
  void QueryOprCmdScriptContent(CwxMsgBlock*& msg, DcmdTss* tss);
  // 获取task-cmd的content
  void QueryTaskCmdScriptContent(CwxMsgBlock*& msg, DcmdTss* tss);
  // 任务进度查询
  void QuerySubTaskProcess(CwxMsgBlock*& msg, DcmdTss* tss);
  // 回复管理系统
  static void ReplyAdmin(DcmdCenterApp* app, uint32_t conn_id, CwxMsgBlock* msg);
  // 获取下一个消息的Task Id
  inline uint32_t NextMsgTaskId() {
    next_msg_task_id_++;
    if (!next_msg_task_id_) next_msg_task_id_++;
    return next_msg_task_id_;
  }

 private:
  // app对象
  DcmdCenterApp*               app_;
  // 消息的id，由于此是单线程执行，因此无需锁保护
  uint32_t                     next_msg_task_id_;
};
}
#endif 
