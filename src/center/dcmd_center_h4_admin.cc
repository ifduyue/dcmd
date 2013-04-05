﻿#include "dcmd_center_h4_admin.h"

#include <CwxMd5.h>

#include "dcmd_center_app.h"
#include "dcmd_center_opr_task.h"
#include "dcmd_center_result_task.h"
#include "dcmd_center_runopr_task.h"
#include "dcmd_center_runtask_task.h"

namespace dcmd {
int DcmdCenterH4Admin::onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv) {
  DcmdTss* pTss = (DcmdTss*)pThrEnv;
  if (!msg) {
    CWX_ERROR(("msg from ui is empty, close connect."));
    app_->noticeCloseConn(msg->event().getConnId());
    return 1;
  }
  if (dcmd_api::MTYPE_UI_EXEC_OPR == msg->event().getMsgHeader().getMsgType()){
    ExecOprCmd(msg, pTss);
  }else if (dcmd_api::MTYPE_UI_AGENT_INFO == msg->event().getMsgHeader().getMsgType()){
    QueryAgentStatus(msg, pTss);
  }else if (dcmd_api::MTYPE_UI_INVALID_AGENT == msg->event().getMsgHeader().getMsgType()){
    QueryIllegalAgent(msg, pTss);
  }else if (dcmd_api::MTYPE_UI_AGENT_SUBTASK_OUTPUT == msg->event().getMsgHeader().getMsgType()){
    QuerySubtaskOutput(msg, pTss);
  }else if (dcmd_api::MTYPE_UI_AGENT_RUNNING_SUBTASK == msg->event().getMsgHeader().getMsgType()){
    QueryAgentRunSubTask(msg, pTss);
  }else if (dcmd_api::MTYPE_UI_AGENT_RUNNING_OPR == msg->event().getMsgHeader().getMsgType()){
    QueryAgentRunOpr(msg, pTss);
  }else if(dcmd_api::MTYPE_UI_OPR_CMD_INFO == msg->event().getMsgHeader().getMsgType()){
    QueryOprCmdScriptContent(msg, pTss);
  }else if (dcmd_api::MTYPE_UI_TASK_CMD_INFO == msg->event().getMsgHeader().getMsgType()){
    QueryTaskCmdScriptContent(msg, pTss);
  }else if (dcmd_api::MTYPE_UI_SUBTASK_PROCESS == msg->event().getMsgHeader().getMsgType()){
    QuerySubTaskProcess(msg, pTss);
  }else{
    CWX_ERROR(("Receive invalid msg type from admin port, msg_type=%d", msg->event().getMsgHeader().getMsgType()));
    app_->noticeCloseConn(msg->event().getConnId());
  }
  return 1;
}

int DcmdCenterH4Admin::onTimeoutCheck(CwxMsgBlock*& , CwxTss* pThrEnv) {
  list<CwxTaskBoardTask*> tasks;
  app_->getTaskBoard().noticeCheckTimeout(pThrEnv, tasks);
  if (!tasks.empty()){
    list<CwxTaskBoardTask*>::iterator iter=tasks.begin();
    while(iter != tasks.end()){
      (*iter)->execute(pThrEnv);
      ++iter;
    }
  }
  return 1;
}

void DcmdCenterH4Admin::ExecOprCmd(CwxMsgBlock*& msg, DcmdTss* tss) {
  DcmdCenterOprTask* opr_task = NULL;
  dcmd_api::UiExecOprCmdReply  opr_cmd_reply;
  dcmd_api::UiExecOprCmd       opr_cmd;
  if (!app_->config().common().is_allow_opr_cmd_) {
    opr_cmd_reply.set_err("Exec opr-cmd is disable.");
    opr_cmd_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyExecOprCmd(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &opr_cmd_reply);
    return;
  }
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!opr_cmd.ParseFromString(tss->proto_str_)) {
    opr_cmd_reply.set_err("Failed to parse opr cmd.");
    opr_cmd_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyExecOprCmd(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &opr_cmd_reply);
    return;
  }
  CWX_DEBUG(("Receive a opr command, opr-cmd-id=%s", opr_cmd.opr_id().c_str()));
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != opr_cmd.user()) ||
    (app_->config().common().ui_user_passwd_ != opr_cmd.passwd()))
  {
    CWX_ERROR(("admin's user[%s] or passwd[%s] is wrong.",
      opr_cmd.user().c_str(),
      opr_cmd.passwd()));
    opr_cmd_reply.set_err("user or passwd is wrong.");
    opr_cmd_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyExecOprCmd(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &opr_cmd_reply);
    return;
  }

  opr_task = new DcmdCenterOprTask(app_, &app_->getTaskBoard());
  opr_task->reply_conn_id_ = msg->event().getConnId();
  opr_task->msg_task_id_ = msg->event().getMsgHeader().getTaskId();
  opr_task->opr_cmd_id_ = strtoull(opr_cmd.opr_id().c_str(), NULL, 10);
  if (opr_cmd.args_size()) {
    dcmd_api::KeyValue
    for (i=0; i< opr_cmd.args_size(); i++) {
      opr_task->opr_args_.push_back(pair<string, string>(opr_cmd.args(i).key,
        opr_cmd.args(i).value));
    }
  }
  opr_task->setTaskId(NextMsgTaskId());
  opr_task->execute(tss);
}

void DcmdCenterH4Admin::QuerySubtaskOutput(CwxMsgBlock*& msg, DcmdTss* tss) {
  DcmdCenterSubtaskOutputTask*  output_task = NULL;
  dcmd_api::UiTaskOutput        subtask_output_;
  dcmd_api::UiTaskOutputReply   subtask_output_reply_;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!subtask_output_.ParseFromString(tss->proto_str_)) {
    subtask_output_reply_.set_err("Failed to parse subtask-result command.");
    subtask_output_reply_.set_state(dcmd_api::FAILED);
    subtask_output_reply_.set_offset(0);
    subtask_output_reply_.set_result("");
    DcmdCenterH4Admin::ReplySubTaskOutput(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &subtask_output_reply_);
    return;
  }
  CWX_DEBUG(("Receive a agent subtask-output command, agent=%s, subtask_id",
    subtask_output_.ip.c_str(), subtask_output_.subtask_id().c_str()));
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != subtask_output_.user()) ||
    (app_->config().common().ui_user_passwd_ != subtask_output_.passwd()))
  {
    CWX_ERROR(("admin's user[%s] or passwd[%s] is wrong.",
      subtask_output_.user().c_str(),
      subtask_output_.passwd().c_str()));
    subtask_output_reply_.set_err("user or passwd is wrong.");
    subtask_output_reply_.set_state(dcmd_api::FAILED);
    subtask_output_reply_.set_offset(0);
    subtask_output_reply_.set_result("");
    DcmdCenterH4Admin::ReplySubTaskOutput(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &subtask_output_reply_);
    return;
  }

  output_task = new DcmdCenterSubtaskOutputTask(app_, &app_->getTaskBoard());
  output_task->reply_conn_id_ = msg->event().getConnId();
  output_task->msg_taskid_ = msg->event().getMsgHeader().getTaskId();
  output_task->agent_ip_ = subtask_output_.ip();
  output_task->subtask_id_ = subtask_output_.subtask_id();
  output_task->output_offset_ = subtask_output_.offset();
  output_task->setTaskId(NextMsgTaskId());
  output_task->execute(tss);
}

void DcmdCenterH4Admin::QueryAgentRunSubTask(CwxMsgBlock*& msg, DcmdTss* tss) {
  dcmd_api::UiAgentRunningTask  running_subtask_query;
  dcmd_api::UiAgentRunningTaskReply  running_subtask_reply;
  DcmdCenterRunSubtaskTask*  subtask_task = NULL;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!running_subtask_query.ParseFromString(tss->proto_str_)) {
    running_subtask_reply.set_err("Failed to parse running-subtask msg.");
    running_subtask_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentRunSubTask(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &running_subtask_reply);
    return;
  }
  CWX_DEBUG(("Receive a run agent task message, agent=%s",
    running_subtask_query.ip().c_str()));
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != running_subtask_query.user()) ||
    (app_->config().common().ui_user_passwd_ != running_subtask_query.passwd()))
  {
    CWX_ERROR(("admin's user[%s] or passwd[%s] is wrong.",
      running_subtask_query.user().c_str(),
      running_subtask_query.passwd().c_str()));
    running_subtask_reply.set_err("User name or password is wrong.");
    running_subtask_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentRunSubTask(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &running_subtask_reply);
    return;
  }
  subtask_task = new DcmdCenterRunSubtaskTask(app_, app_->getTaskBoard());
  subtask_task->reply_conn_id_ = msg->event().getConnId();
  subtask_task->msg_taskid_ = msg->event().getMsgHeader().getTaskId();
  subtask_task->agent_ip_ = running_subtask_query.ip();
  subtask_task->svr_name_ = running_subtask_query.svr_name();
  subtask_task->setTaskId(NextMsgTaskId());
  subtask_task->execute(tss);
}

// run shell的查询消息
void DcmdCenterH4Admin::QueryAgentRunOpr(CwxMsgBlock*& msg, DcmdTss* tss) {
  dcmd_api::UiAgentRunningOpr  running_opr_query;
  dcmd_api::UiAgentRunningOprReply  running_opr_reply;
  DcmdCenterRunOprTask*  opr_task = NULL;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!running_opr_query.ParseFromString(tss->proto_str_)) {
    running_opr_reply.set_err("Failed to parse running-opr msg.");
    running_opr_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentRunOprCmd(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &running_opr_reply);
    return;
  }
  CWX_DEBUG(("Receive a query agent running opr-cmd message, agent=%s",
    running_opr_query.ip().c_str()));
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != running_opr_query.user()) ||
    (app_->config().common().ui_user_passwd_ != running_opr_query.passwd()))
  {
    CWX_ERROR(("admin's user[%s] or passwd[%s] is wrong.",
      running_opr_query.user().c_str(),
      running_opr_query.passwd().c_str()));
    running_opr_reply.set_err("User name or password is wrong.");
    running_opr_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentRunOprCmd(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &running_opr_reply);
    return;
  }
  opr_task = new DcmdCenterRunOprTask(app_, &app_->getTaskBoard());
  opr_task->reply_conn_id_ = msg->event().getConnId();
  opr_task->msg_taskid_ = msg->event().getMsgHeader().getTaskId();
  opr_task->agent_ip_ = running_opr_query.ip();
  opr_task->setTaskId(NextMsgTaskId());
  opr_task->execute(tss);
}

void DcmdCenterH4Admin::QueryAgentStatus(CwxMsgBlock*& msg, DcmdTss* tss){
  dcmd_api::UiAgentInfo  agent_info_query;
  dcmd_api::UiAgentInfoReply  agent_info_reply;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!agent_info_query.ParseFromString(tss->proto_str_)) {
    agent_info_reply.set_err("Failed to parse agent-info msg.");
    agent_info_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentStatus(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &agent_info_reply);
    return;
  }
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != agent_info_query.user()) ||
    (app_->config().common().ui_user_passwd_ != agent_info_query.passwd()))
  {
    agent_info_reply.set_err("User or password is wrong.");
    agent_info_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentStatus(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &agent_info_reply);
    return;
  }
  CWX_DEBUG(("Receive a agent status query message"));
  // 获取 agent 的状态
  list<string> agent_ips;
  for (int i=0; i<agent_info_query.ips_size(); i++) {
    agent_ips.push_back(agent_info_query.ips(i));
  }
  app_->GetAgentMgr()->GetAgentStatus(agent_ips,
    agent_info_query.version(),
    agent_info_reply);
  DcmdCenterH4Admin::ReplyAgentStatus(app_,
    tss,
    msg->event().getConnId(),
    msg->event().getMsgHeader().getTaskId(),
    &agent_info_reply);
}

void DcmdCenterH4Admin::QueryIllegalAgent(CwxMsgBlock*& msg, DcmdTss* tss) {
  dcmd_api::UiInvalidAgentInfo  invalid_agent_query;
  dcmd_api::UiInvalidAgentInfoReply  invalid_agent_reply;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!invalid_agent_query.ParseFromString(tss->proto_str_)) {
    invalid_agent_reply.set_err("Failed to parse invalid-agent msg.");
    invalid_agent_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyIllegalAgent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &invalid_agent_reply);
    return;
  }
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != invalid_agent_query.user()) ||
    (app_->config().common().ui_user_passwd_ != invalid_agent_query.passwd()))
  {
    invalid_agent_reply.set_err("User or password is wrong.");
    invalid_agent_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyIllegalAgent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &invalid_agent_reply);
    return;
  }
  CWX_DEBUG(("Receive a  illegal agent query"));
  app_->GetAgentMgr()->GetInvalidAgent(invalid_agent_reply);
  DcmdCenterH4Admin::ReplyIllegalAgent(app_,
    tss,
    msg->event().getConnId(),
    msg->event().getMsgHeader().getTaskId(),
    &invalid_agent_reply);
}

void DcmdCenterH4Admin::QueryOprCmdScriptContent(CwxMsgBlock*& msg, DcmdTss* tss) {
  dcmd_api::UiOprScriptInfo  opr_script_query;
  dcmd_api::UiOprScriptInfoReply  opr_script_reply;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!opr_script_query.ParseFromString(tss->proto_str_)) {
    opr_script_reply.set_err("Failed to parse opr script msg.");
    opr_script_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyOprScriptContent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &opr_script_reply);
    return;
  }
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != opr_script_query.user()) ||
    (app_->config().common().ui_user_passwd_ != opr_script_query.passwd()))
  {
    opr_script_reply.set_err("User or password is wrong.");
    opr_script_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyOprScriptContent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &opr_script_reply);
    return;
  }
  // 获取文件内容
  string opr_cmd_file;
  DcmdCenterConf::opr_cmd_file(opr_script_query.opr_file(), opr_cmd_file);
  string content;
  string content_md5;
  string err_msg;
  if (!DcmdTss::ReadFile(opr_cmd_file.c_str(), content, err_msg)) {
    CwxCommon::snprintf(tss->m_szBuf2K, 2047,
      "Failure to read script for opr-cmd[%s]. file:%s, err:%s",
      opr_script_query.opr_file().c_str(),
      opr_cmd_file.c_str(),
      err_msg.c_str());
    CWX_ERROR((tss->m_szBuf2K));
    opr_script_reply.set_err(tss->m_szBuf2K);
    opr_script_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyOprScriptContent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &opr_script_reply);
    return;
  }
  // 计算md5
  dcmd_md5(content.c_str(), content.length(), content_md5);
  opr_script_reply.set_state(dcmd_api::SUCCESS);
  opr_script_reply.set_md5(content_md5);
  opr_script_reply.set_script(content);
  DcmdCenterH4Admin::ReplyOprScriptContent(app_,
    tss,
    msg->event().getConnId(),
    msg->event().getMsgHeader().getTaskId(),
    &opr_script_reply);
}

void DcmdCenterH4Admin::QueryTaskCmdScriptContent(CwxMsgBlock*& msg, DcmdTss* tss) {
  dcmd_api::UiTaskScriptInfo  task_script_query;
  dcmd_api::UiTaskScriptInfoReply  task_script_reply;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!task_script_query.ParseFromString(tss->proto_str_)) {
    task_script_reply.set_err("Failed to parse task script msg.");
    task_script_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyTaskCmdScriptContent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &task_script_reply);
    return;
  }
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != task_script_query.user()) ||
    (app_->config().common().ui_user_passwd_ != task_script_query.passwd()))
  {
    task_script_reply.set_err("User or password is wrong.");
    task_script_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyTaskCmdScriptContent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &task_script_reply);
    return;
  }
  // 获取文件内容
  string task_cmd_file;
  DcmdCenterConf::task_cmd_file(task_script_query.task_cmd(), task_cmd_file);
  string content;
  string content_md5;
  string err_msg;
  if (!DcmdTss::ReadFile(task_cmd_file.c_str(), content, err_msg)) {
    CwxCommon::snprintf(tss->m_szBuf2K, 2047,
      "Failure to read script for task-cmd[%s]. file:%s, err:%s",
      task_script_query.task_cmd().c_str(),
      task_cmd_file.c_str(),
      err_msg.c_str());
    CWX_ERROR((tss->m_szBuf2K));
    task_script_reply.set_err(tss->m_szBuf2K);
    task_script_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyTaskCmdScriptContent(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &task_script_reply);
    return;
  }
  // 计算md5
  dcmd_md5(content.c_str(), content.length(), content_md5);
  task_script_reply.set_state(dcmd_api::SUCCESS);
  task_script_reply.set_md5(content_md5);
  task_script_reply.set_script(content);
  DcmdCenterH4Admin::ReplyTaskCmdScriptContent(app_,
    tss,
    msg->event().getConnId(),
    msg->event().getMsgHeader().getTaskId(),
    &task_script_reply);
}

void DcmdCenterH4Admin::QuerySubTaskProcess(CwxMsgBlock*& msg, DcmdTss* tss) {
  dcmd_api::UiAgentTaskProcess  task_process_query;
  dcmd_api::UiAgentTaskProcessReply  task_process_reply;
  tss->proto_str_.assign(msg->rd_ptr(), msg->length());
  if (!task_process_query.ParseFromString(tss->proto_str_)) {
    task_process_reply.set_err("Failed to parse task process msg.");
    task_process_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentSubTaskProcess(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &task_process_reply);
    return;
  }
  // 检查用户名与密码
  if ((app_->config().common().ui_user_name_ != task_process_query.user()) ||
    (app_->config().common().ui_user_passwd_ != task_process_query.passwd()))
  {
    task_process_reply.set_err("User or password is wrong.");
    task_process_reply.set_state(dcmd_api::FAILED);
    DcmdCenterH4Admin::ReplyAgentSubTaskProcess(app_,
      tss,
      msg->event().getConnId(),
      msg->event().getMsgHeader().getTaskId(),
      &task_process_reply);
    return;
  }
  // 查询agent的任务处理进度
  string process;
  for (int i=0; i<task_process_query.subtask_id_size(); i++) {
    app_->GetTaskMgr()->GetAgentsTaskProcess(task_process_query->subtask_id(i), process);
    task_process_reply.add_process(process);
  }
  task_process_reply.set_state(dcmd_api::SUCCESS);
  DcmdCenterH4Admin::ReplyAgentSubTaskProcess(app_,
    tss,
    msg->event().getConnId(),
    msg->event().getMsgHeader().getTaskId(),
    &task_process_reply);
}

void DcmdCenterH4Admin::ReplyExecOprCmd(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiExecOprCmdReply* result)
{
    CwxMsgBlock* block = NULL;
    if (!result->SerializeToString(&tss->proto_str_)) {
      CWX_ERROR(("Failure to pack Opr cmd msg."));
      app->noticeCloseConn(conn_id);
      return;
    }
    CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_EXEC_OPR_R, msg_task_id,
      tss->proto_str_.length());
    msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
      tss->proto_str_.length());
    if (!msg) {
      CWX_ERROR(("Failure to pack opr cmd msg for no memory"));
      exit(1);
    }
    ReplyAdmin(app, conn_id, block);
}

void DcmdCenterH4Admin::ReplySubTaskOutput(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiTaskOutputReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack subtask output msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_AGENT_SUBTASK_OUTPUT_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack subtask output msg for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

void DcmdCenterH4Admin::ReplyAgentRunSubTask(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiAgentRunningTaskReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack running subtask msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_AGENT_RUNNING_SUBTASK_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack running subtask msg for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

///回复run shell结果查询
void DcmdCenterH4Admin::ReplyAgentRunOprCmd(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiAgentRunningOprReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack running opr msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_AGENT_RUNNING_OPR_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack running opr msg for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

void DcmdCenterH4Admin::ReplyAgentStatus(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiAgentInfoReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack agent status msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_AGENT_INFO_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack agent status msg for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

///回复查询非法的agent
void DcmdCenterH4Admin::ReplyIllegalAgent(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiInvalidAgentInfoReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack invalid agent msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_INVALID_AGENT_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack invalid agent for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

void DcmdCenterH4Admin::ReplyOprScriptContent(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiOprScriptInfoReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack opr cmd script msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_OPR_CMD_INFO_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack opr script msg for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

void DcmdCenterH4Admin::ReplyTaskCmdScriptContent(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiTaskScriptInfoReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack task cmd script msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_TASK_CMD_INFO_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack task cmd script msg for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

void DcmdCenterH4Admin::ReplyAgentSubTaskProcess(DcmdCenterApp* app,
  DcmdTss* tss,
  uint32_t conn_id,
  uint32_t msg_task_id,
  dcmd_api::UiAgentTaskProcessReply* result)
{
  CwxMsgBlock* block = NULL;
  if (!result->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack task process msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_UI_SUBTASK_PROCESS_R, msg_task_id,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack task process msg for no memory"));
    exit(1);
  }
  ReplyAdmin(app, conn_id, block);
}

void DcmdCenterH4Admin::ReplyAdmin(DcmdCenterApp* app, uint32_t conn_id, CwxMsgBlock* msg)
{
  msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_ADMIN);
  msg->send_ctrl().setHostId(0);
  msg->send_ctrl().setConnId(conn_id);
  msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
  if (0 != app->sendMsgByConn(msg)) {
    CwxMsgBlockAlloc::free(msg);
    CWX_ERROR(("Failure to send msg to admin, close connect."));
    app->noticeCloseConn(conn_id);
  }
}
}  // dcmd