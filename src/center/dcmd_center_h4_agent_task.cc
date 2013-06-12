#include "dcmd_center_h4_agent_task.h"

#include <CwxMd5.h>

#include "dcmd_center_app.h"
#include "dcmd_center_h4_check.h"

namespace dcmd {
int DcmdCenterH4AgentTask::onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv) {
  DcmdTss* tss = (DcmdTss*)pThrEnv;
  string conn_ip = "";
  app_->GetAgentMgr()->GetConnIp(msg->event().getConnId(), conn_ip);
  switch(msg->event().getMsgHeader().getMsgType()){
  case dcmd_api::MTYPE_AGENT_REPORT:
    AgentReport(msg, tss);
    break;
  case dcmd_api::MTYPE_AGENT_HEATBEAT:
    // 此消息已经有通信线程处理
    CWX_ASSERT(0);
    break;
  case dcmd_api::MTYPE_CENTER_MASTER_NOTICE_R:
    AgentMasterReply(msg, tss);
    break;
  case dcmd_api::MTYPE_CENTER_SUBTASK_CMD_R:
    AgentSubtaskAccept(msg, tss);
    break;
  case dcmd_api::MTYPE_AGENT_SUBTASK_CMD_RESULT:
    AgentSubtaskResult(msg, tss);
    break;
  case dcmd_api::MTYPE_AGENT_SUBTASK_PROCESS:
    AgentSubtaskProcess(msg, tss);
    break;
  case dcmd_api::MTYPE_UI_EXEC_TASK:
    UiExecTaskCmd(msg, tss);
    break;
  default:
    CWX_ERROR(("Receive invalid msg type[%d] from host:%s, ignore it.",
      msg->event().getMsgHeader().getMsgType(),
      conn_ip.length()?conn_ip.c_str():"unknown"));
    break;
  }
  return 1;
}

// 连接关闭后，需要清理环境
int DcmdCenterH4AgentTask::onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv) {
  DcmdTss* tss = (DcmdTss*)pThrEnv;
  string agent_ip;
  if (msg->event().getConnUserData()) {
    agent_ip = (char*) msg->event().getConnUserData();
    free(msg->event().getConnUserData());
    app_->GetTaskMgr()->ReceiveAgentClosed(tss, agent_ip);
  }
  return 1;
}

// 检查同步的超时
int DcmdCenterH4AgentTask::onTimeoutCheck(CwxMsgBlock*& , CwxTss* pThrEnv) {
  DcmdTss* tss = (DcmdTss*)pThrEnv;
  static uint32_t last_check_time = time(NULL);
  static uint32_t base_time = 0;
  uint32_t now = time(NULL);
  bool is_clock_back = app_->isClockBack(base_time, now);
  if (!is_clock_back && (now <= last_check_time)) return 1;
  last_check_time = now;
  if (app_->is_master()()) {
    if (!is_master_) {
      CWX_INFO(("I becomes master, startup task manager......"));
      is_master_ = app_->GetTaskMgr()->Start(tss);
      if (!is_master_) {
        CWX_ERROR(("Failed to start task manager."));
      } else {
        // 通知所有agent，自己是master
        CWX_INFO(("Notice all agent that i becomes master......"));
        NoticeMaster(pTss, NULL);
      }
    }
  }
  return 1;
}

int DcmdCenterH4AgentTask::onUserEvent(CwxMsgBlock*& , CwxTss* pThrEnv){
  DcmdTss* tss = (DcmdTss*)pThrEnv;
  CWX_INFO(("Receive master change event. app master:%s, handle master:%s",
    app_->is_master()?"true":"false",
    is_master_?"true":"false"));
  if (app_->is_master()){
    // 由于是不同的链接，防止master切换带来问题
    CWX_DEBUG(("Start check task mysql..........."));
    if (!app_->CheckMysql(app_->GetTaskMysql())) return 1;
    string host;
    if (!DcmdCenterH4Check::GetMasterHost(app_->GetTaskMysql(), host, tss)){
      // 关闭mysql的连接
      app_->GetTaskMysql()->disconnect();
      return 1;
    }
    if (host != app_->config().common().host_id_){
      app_->GetTaskMysql()->disconnect();
      return 1;
    }
    if (!is_master_){
      CWX_INFO(("Startup task manager......."));
      is_master_ = app_->GetTaskMgr()->Start();
      if (!is_master_){
        CWX_ERROR(("Failure to start task manager."));
      }else{
        ///通知所有agent，自己是master
        CWX_INFO(("Notice all agent that i am master......"));
        NoticeMaster(tss, NULL);
      }
    }
  }else{
    CWX_INFO(("Stop task manager."));
    app_->GetTaskMgr()->Stop(tss);
    app_->GetAgentMgr()->ClearMasterNoticeReportReply();
    is_master_ = false;
  }
  return 1;
}

void DcmdCenterH4AgentTask::ReplyAgentReport(DcmdCenterApp* app,
  DcmdTss*      tss,
  uint32_t      conn_id,
  uint32_t      msg_taskid,
  dcmd_api::AgentReportReply const* reply)
{
  CwxMsgBlock* msg = NULL;
  if (!reply->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack agent report msg."));
    app->noticeCloseConn(conn_id);
    return;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_AGENT_REPORT_R, msg_taskid,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack agent report reply msg for no memory"));
    exit(1);
  }
  msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_AGENT);
  msg->send_ctrl().setConnId(conn_id);
  msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
  if (0 != app->sendMsgByConn(msg)){
    CwxMsgBlockAlloc::free(msg);
    CWX_ERROR(("Failure to send msg to agent:%s, close connect.", strAgentIp.c_str()));
    app->noticeCloseConn(conn_id);
  }
}

// false：发送失败；true：发送成功。
bool DcmdCenterH4AgentTask::SendAgentCmd(DcmdCenterApp* app,
  DcmdTss* tss,
  string const& agent_ip,
  uint32_t msg_taskid,
  dcmd_api::AgentTaskCmd const* cmd)
{
  CwxMsgBlock* msg = NULL;
  if (!reply->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack agent report msg."));
    app->noticeCloseConn(conn_id);
    return false;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_CENTER_SUBTASK_CMD, msg_taskid,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack agent report reply msg for no memory"));
    exit(1);
  }
  msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_AGENT);
  msg->send_ctrl().setConnId(conn_id);
  msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
  if (0 != app->sendMsgByConn(msg)){
    CwxMsgBlockAlloc::free(msg);
    CWX_ERROR(("Failure to send msg to agent:%s, close connect.", strAgentIp.c_str()));
    app->noticeCloseConn(conn_id);
    return false;
  }
  return true;
}

// false：发送失败；true：发送成功
bool DcmdCenterH4AgentTask::ReplyAgentCmdResult(DcmdCenterApp* app,
                                 DcmdTss* tss,
                                 string const& agent_ip,
                                 uint32_t msg_taskid,
                                 dcmd_api::AgentTaskResultReply const* reply)
{
  CwxMsgBlock* msg = NULL;
  if (!reply->SerializeToString(&tss->proto_str_)) {
    CWX_ERROR(("Failure to pack agent report msg."));
    app->noticeCloseConn(conn_id);
    return false;
  }
  CwxMsgHead head(0, 0, dcmd_api::MTYPE_CENTER_SUBTASK_CMD, msg_taskid,
    tss->proto_str_.length());
  msg = CwxMsgBlockAlloc::pack(head, tss->proto_str_.c_str(),
    tss->proto_str_.length());
  if (!msg) {
    CWX_ERROR(("Failure to pack agent report reply msg for no memory"));
    exit(1);
  }
  msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_AGENT);
  msg->send_ctrl().setConnId(conn_id);
  msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
  uint32_t conn_id = 0;
  if (!app->GetAgentMgr()->SendMsg(agent_ip, msg, conn_id)){
    CwxMsgBlockAlloc::free(msg);
    CWX_ERROR(("Failure to send msg to agent:%s.", agent_ip.c_str()));
    return false;
  }
  return true;
}

// agent报告自己的状态
void  DcmdCenterH4AgentTask::AgentReport(CwxMsgBlock*& msg, DcmdTss* tss){
  string agent_ips;
  string conn_ip="";
  string agent_ip;
  list<string> ips;
  bool is_success = false;
  string err_msg;
  dcmd_api::AgentReport report;
  // 连接已经不存在
  if (!app_->GetAgentMgr()->GetConnIp(msg->event().getConnId(), conn_ip)) return;
  do{
    tss->proto_str_.assign(msg->rd_ptr(), msg->length());
    if (!report.ParseFromString(tss->proto_str_)) {
      CWX_ERROR(("Failure unpack agent report msg from %s, close it.", conn_ip.c_str()));
      err_msg = "Failure unpack agent report msg";
      break;
    }
    for (uint32_t i=0; i<report.agent_ips_size(); i++) {
      if (agent_ips.length()) {
        agent_ips += ",";
      }
      agent_ips += report.agent_ips(i);
      ips.push_back(report.agent_ips(i));
    }
    CWX_DEBUG(("Receive agent[%s]'s report from %s", agent_ips.c_str(), conn_ip.c_str())); 
    if (!app_->GetAgentMgr()->ComfirmAgentIpByReportedIp(ips, agent_ip)){
      CWX_ERROR(("report agent ip:%s isn't registered , close it.", agent_ips.c_str()));
      err_msg = "report agent ip isn't registered";
      ///添加到无效的agent连接中
      app_->GetAgentMgr()->AddInvalidConn(conn_ip, agent_ips);
      break;
    }
    //鉴权
    string old_conn_ip;
    uint32_t old_conn_id=0;
    int ret = app_->GetAgentMgr()->Auth(msg->event().getConnId(),
      agent_ip,
      report.version(),
      agent_ips,
      old_conn_ip,
      old_conn_id);
    if (2 == ret){///连接已经关闭
      CWX_ERROR(("Agent[%s] is closed.", agent_ip.c_str()));
      return;
    }
    if (1 == ret){//auth ip exist，可能是非法冒充ip，也可能是重连
      if (old_conn_ip == conn_ip){//关闭旧连接
        app_->GetAgentMgr()->UnAuth(agent_ip);
        app_->GetTaskMgr()->ReceiveAgentClosed(tss, agent_ip);
        app_->noticeCloseConn(old_conn_id);
        ///重新认证
        ret = app_->GetAgentMgr()->Auth(msg->event().getConnId(),
          agent_ip,
          report.version(),
          agent_ips,
          old_conn_ip,
          old_conn_id);
        CWX_INFO(("Connection for agent[%s] is duplicate, close the old", agent_ip.c_str()));
        CWX_ASSERT(1 != ret); ///可能为2，连接不存在
      }else if(conn_ip == agent_ip){///当前的连接就是正确的
        app_->GetAgentMgr()->UnAuth(agent_ip);
        app_->GetTaskMgr()->ReceiveAgentClosed(tss, agent_ip);
        app_->noticeCloseConn(old_conn_id);
        ///重新认证
        ret = app_->GetAgentMgr()->Auth(msg->event().getConnId(),
          agent_ip,
          report.version(),
          agent_ips,
          old_conn_ip,
          old_conn_id);
        CWX_INFO(("Old conn[%s] for agent[%s] is invalid, close the old", old_conn_ip.c_str(), agent_ip.c_str()));
        CWX_ASSERT(1 != ret); ///可能为2，连接不存在
        if (2 == ret) return; ///连接不存在
      } else {
        CWX_INFO(("conn[%s] for agent[%s] is invalid, close it", conn_ip.c_str(), agent_ip.c_str()));
        err_msg = "Failure to auth";
        app_->GetAgentMgr()->AddInvalidConn(conn_ip,agent_ips);
        break;
      }
    }
    is_success = true;
  }while(0);

  // 回复
  dcmd_api::AgentReportReply  reply;
  reply.set_state(is_success?dcmd_api::DCMD_STATE_SUCCESS:dcmd_api::DCMD_STATE_FAILED);
  reply.set_err(is_success?err_msg:"");
  reply.set_heatbeat(app_->config().common().heatbeat_internal_);
  reply.set_package_size(app_->config().common().agent_package_size_);
  reply.set_opr_overflow_threshold(app_->config().common().opr_overflow_threshold_);
  reply.set_opr_queue_threshold(app_->config().common().opr_queue_threshold_);
  ReplyAgentReport(app_,
    tss,
    msg->event().getConnId(),
    msg->event().getMsgHeader().getTaskId(),
    &reply);
  // 如果自己是master，则通知agent
  if (is_success && app_->is_master())  noticeMaster(tss, &agent_ip);
  return;
}

///agent报告自己的任务处理状态
void  DcmdCenterH4AgentTask::AgentMasterReply(CwxMsgBlock*& msg, DcmdTss* tss){
    char const* szCmdIds = NULL;
    string strConnIp="";
    string strAgentIp = "";
    if (!m_pApp->getAgentMgr()->getConnIp(msg->event().getConnId(), strConnIp)){///连接已经关闭
        return;
    }
    if (!m_pApp->getAgentMgr()->getAgentIp(msg->event().getConnId(), strAgentIp)){
        ///若没有认证，则直接关闭连接
        CWX_ERROR(("Close agent connection with conn-ip[%s] for recieving master-reply but no auth. ", strConnIp.c_str()));
        m_pApp->noticeCloseConn(msg->event().getConnId());
        return;
    }

    if (!m_pApp->isMaster()){
        CWX_DEBUG(("I'm not master, ignore agent's master-reply."));
        m_pApp->getTaskMgr()->stop(tss);
        return; ///若不是master，直接忽略
    }
    
    if (DCMD_ERR_SUCCESS != DcmdPoco::parseCenterMasterNoticeReply(tss->m_pReader,
        msg,
        szCmdIds,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure unpack agent's master-notice reply msg from %s, close it. err:%s", strConnIp.c_str(), tss->m_szBuf2K));
        m_pApp->noticeCloseConn(msg->event().getConnId());
        return;
    }
    CWX_DEBUG(("Receive agent's master-reply, agent:%s, cmd_id=%s", strAgentIp.c_str(), szCmdIds));

    int ret = m_pApp->getAgentMgr()->masterNoticeReply(msg->event().getConnId());
    if (2 == ret) {///连接已经关闭
        return;
    }
    if (1 == ret){///没有报告自己的agent信息
        CWX_ERROR(("agent[%s] doesn't report self-agent info it.", strConnIp.c_str()));
        m_pApp->noticeCloseConn(msg->event().getConnId());
        return;
    }
    ///通知task manager，agent已经建立可以发送消息了
    string strCmdIds(szCmdIds);
    m_pApp->getTaskMgr()->agentMasterReply(tss, strAgentIp, strCmdIds);
}

///agent报告自己已经接受子任务
void  DcmdCenterH4AgentTask::agentTaskAcccept(CwxMsgBlock*& msg, DcmdTss* tss){
    if (!m_pApp->isMaster()){
        return; ///若不是master，直接忽略
    }
    string strAgentIp="";
    if (!m_pApp->getAgentMgr()->getAgentIp(msg->event().getConnId(), strAgentIp)){///连接已经关闭
        return;   
    }
    CWX_UINT64 ullCmdId = 0;
    if (DCMD_ERR_SUCCESS != DcmdPoco::parseTaskCmdReply(tss->m_pReader,
        msg,
        ullCmdId,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure unpack agent's task reply msg from %s, close it. err:%s", strAgentIp.c_str(), tss->m_szBuf2K));
        m_pApp->noticeCloseConn(msg->event().getConnId());
        return;
    }
    char szTmp[128];
    CWX_DEBUG(("Receive agent's command-confirm message, agent:%s, cmd_id=%s",
        strAgentIp.c_str(), 
        CwxCommon::toString(ullCmdId, szTmp, 0)));
    m_pApp->getTaskMgr()->agentNoticeConfirm(tss, strAgentIp, ullCmdId);

}

///agent回复任务的处理结果
void  DcmdCenterH4AgentTask::agentTaskResult(CwxMsgBlock*& msg, DcmdTss* tss){
    if (!m_pApp->isMaster()){
        return; ///若不是master，直接忽略
    }
    string strAgentIp="";
    if (!m_pApp->getAgentMgr()->getAgentIp(msg->event().getConnId(), strAgentIp)){///连接已经关闭
        return;
    }
    DcmdTaskCmdResult result;
    if (DCMD_ERR_SUCCESS != DcmdPoco::parseTaskCmdResult(tss->m_pReader,
        msg,
        result,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure unpack agent's task result msg, close it. err:%s",tss->m_szBuf2K));
        m_pApp->noticeCloseConn(msg->event().getConnId());
        return;
    }
    char szTmp1[128];
    char szTmp2[128];
    CWX_DEBUG(("Receive agent's command result, agent:%s, task_id=%u tasknode-id:%s cmd_id=%s",
        strAgentIp.c_str(), 
        result.m_uiTaskId,
        CwxCommon::toString(result.m_ullTaskNodeId, szTmp1, 10),
        CwxCommon::toString(result.m_ullCmdId, szTmp2, 10)));

    if (!m_pApp->getTaskMgr()->agentNoticeResult(tss, msg->event().getMsgHeader().getTaskId(), result)){
        ///关闭连接以便再处理
        m_pApp->noticeCloseConn(msg->event().getConnId());
    }
}

///agent报告任务的处理进度
void DcmdCenterH4AgentTask::agentTaskProcess(CwxMsgBlock*& msg, DcmdTss* tss){
    if (!m_pApp->isMaster()){
        return; ///若不是master，直接忽略
    }
    string strAgentIp="";
    if (!m_pApp->getAgentMgr()->getAgentIp(msg->event().getConnId(), strAgentIp)){///连接已经关闭
        return;
    }
    CWX_UINT32 uiTid = 0;
    CWX_UINT64 ullTaskNodeId = 0;
    char* szProcess = NULL;
    if (DCMD_ERR_SUCCESS != DcmdPoco::parseTaskCmdProcess(tss->m_pReader,
        msg,
        uiTid,
        ullTaskNodeId,
        szProcess,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure unpack agent's task process msg. err:%s",tss->m_szBuf2K));
        return;
    }
    char szTmp1[128];
    CWX_DEBUG(("Receive agent's task process, agent:%s, task_id=%u tasknode-id:%s  process=%s",
        strAgentIp.c_str(), 
        uiTid,
        CwxCommon::toString(ullTaskNodeId, szTmp1, 10),
        szProcess?szProcess:""));
    m_pApp->getTaskMgr()->setAgentTaskProcess(uiTid, ullTaskNodeId, szProcess);
}

void DcmdCenterH4AgentTask::noticeMaster(DcmdTss* tss, string const* strAgentIp){
    CwxMsgBlock* msg = NULL;
    CWX_DEBUG(("I am master, notice agent[%s]", strAgentIp?strAgentIp->c_str():"all"));

    if (DCMD_ERR_SUCCESS != DcmdPoco::packCenterMasterNotice(tss->m_pWriter,
        msg,
        0,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to pack master notice msg, err:%s", tss->m_szBuf2K));
        CWX_ASSERT(0);
    }
    msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_AGENT);
    msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    CWX_UINT32 uiConnId = 0;
    if (!strAgentIp){
        m_pApp->getAgentMgr()->broadcastMsg(msg);
    }
    else{
        if (!m_pApp->getAgentMgr()->sendMsg(*strAgentIp, msg, uiConnId)){
            CwxMsgBlockAlloc::free(msg);
        }
    }
}
}  // dcmd