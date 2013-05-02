﻿#include "dcmd_center_h4_agent_task.h"

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
  case dcmd_api::MTYPE_UI_WATCH_TASK:
    UiWatchTaskCmd(msg, tss);
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
    DcmdTss* pTss = (DcmdTss*)pThrEnv;
    CWX_INFO(("Receive master change event. app master:%s, handle master:%s",
        m_pApp->isMaster()?"true":"false",
        m_bMaster?"true":"false"));
    if (m_pApp->isMaster()){
        ///由于是不同的链接，防止master切换带来问题
        CWX_DEBUG(("Start check task mysql..........."));
        if (!m_pApp->checkMysql(m_pApp->getTaskMysql())) return 1;
        string strHost;
        if (!DcmdCenterH4Check::getMasterHost(m_pApp->getTaskMysql(), strHost, pTss)){
            ///关闭mysql的连接
            m_pApp->getTaskMysql()->disconnect();
            return 1;
        }
        if (strHost != m_pApp->getConfig().getCommon().m_strHost){
            m_pApp->getTaskMysql()->disconnect();
            return 1;
        }
        if (!m_bMaster){
            CWX_INFO(("Startup task manager......."));
            m_bMaster = m_pApp->getTaskMgr()->start(pTss);
            if (!m_bMaster){
                CWX_ERROR(("Failure to start task manager."));
            }else{
                ///通知所有agent，自己是master
                CWX_INFO(("Notice all agent that i am master......"));
                noticeMaster(pTss, NULL);
            }
        }
    }else{
        CWX_INFO(("Stop task manager."));
        m_pApp->getTaskMgr()->stop(pTss);
        m_pApp->getAgentMgr()->ClearMasterNoticeReportReply();
        m_bMaster = false;
    }
    return 1;
}

void DcmdCenterH4AgentTask::replyAgentReport(DcmdCenterApp* app,
                                                  DcmdTss* tss,
                                                  CWX_UINT32 uiConnId,
                                                  CWX_UINT32 uiTaskId,
                                                  bool bSuccess,
                                                  CWX_UINT32 uiHeatbeat,
                                                  CWX_UINT32 uiPMsize,
                                                  char const* szErrMsg,
                                                  string const& strAgentIp)
{
    CwxMsgBlock* msg = NULL;
    if (DCMD_ERR_SUCCESS != DcmdPoco::packAgentReportReply(tss->m_pWriter,
        msg,
        uiTaskId,
        bSuccess?0:1,
        szErrMsg,
        uiHeatbeat,
        uiPMsize,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to package agent report reply, err=%s", tss->m_szBuf2K));
        app->noticeCloseConn(uiConnId);
        return;
    }
    msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_AGENT);
    msg->send_ctrl().setConnId(uiConnId);
    msg->send_ctrl().setMsgAttr(bSuccess?CwxMsgSendCtrl::NONE:CwxMsgSendCtrl::CLOSE_NOTICE);
    
    if (0 != app->sendMsgByConn(msg)){
        CwxMsgBlockAlloc::free(msg);
        CWX_ERROR(("Failure to send msg to agent:%s, close connect.", strAgentIp.c_str()));
        app->noticeCloseConn(uiConnId);
    }
}

///false：发送失败；true：发送成功。
bool DcmdCenterH4AgentTask::sendAgentTask(DcmdCenterApp* app,
                          DcmdTss* tss,
                          string const& strAgentIp,
                          CWX_UINT32 uiTaskId,
                          DcmdTaskCmd const& notice,
                          string const& strScript,
                          map<string, string> const& taskArgs)
{
    CwxMsgBlock* msg = NULL;
    if (DCMD_ERR_SUCCESS != DcmdPoco::packTaskCmd(tss->m_pWriter,
        tss->m_pWriter1,
        msg,
        uiTaskId,
        notice,
        strScript.length()?strScript.c_str():NULL,
        strScript.length(),
        taskArgs,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to package task, err=%s", tss->m_szBuf2K));
        app->getAgentMgr()->closeAgent(strAgentIp);
        return false;
    }
    msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_AGENT);
    msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    CWX_UINT32 uiConnId = 0;
    if (!app->getAgentMgr()->sendMsg(strAgentIp, msg, uiConnId)){
        CwxMsgBlockAlloc::free(msg);
        CWX_ERROR(("Failure to send task to agent:%s.", strAgentIp.c_str()));
        return false;
    }
    return true;
}

///false：发送失败；true：发送成功
bool DcmdCenterH4AgentTask::replyAgentTaskResult(DcmdCenterApp* app,
                                 DcmdTss* tss,
                                 string const& strAgentIp,
                                 CWX_UINT32 uiTaskId,
                                 CWX_UINT64 ullCmdId)
{
    CwxMsgBlock* msg = NULL;
    if (DCMD_ERR_SUCCESS != DcmdPoco::packTaskCmdResultReply(tss->m_pWriter,
        msg,
        uiTaskId,
        ullCmdId,
        tss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to package task result reply, err=%s", tss->m_szBuf2K));
        app->getAgentMgr()->closeAgent(strAgentIp);
        return false;
    }
    msg->send_ctrl().setSvrId(DcmdCenterApp::SVR_TYPE_AGENT);
    msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    CWX_UINT32 uiConnId = 0;
    if (!app->getAgentMgr()->sendMsg(strAgentIp, msg, uiConnId)){
        CwxMsgBlockAlloc::free(msg);
        CWX_ERROR(("Failure to send msg to agent:%s.", strAgentIp.c_str()));
        return false;
    }
    return true;
}

///agent报告自己的状态
void  DcmdCenterH4AgentTask::agentReport(CwxMsgBlock*& msg, DcmdTss* tss){
    char const* szAgentIps = NULL;
    string strAgentIps;
    char const* szVer = NULL;
    string strConnIp="";
    string strAgentIp;
    string strIpList;
    list<string> ips;
    bool bRefresh = false;
    bool bSuccess = true;
    string strErr;

    ///连接已经不存在
    if (!m_pApp->getAgentMgr()->getConnIp(msg->event().getConnId(), strConnIp)){///连接已经关闭
        return;
    }
    do{
        if (DCMD_ERR_SUCCESS != DcmdPoco::parseAgentReport(tss->m_pReader,
            msg,
            szAgentIps,
            szVer,
            tss->m_szBuf2K))
        {
            CWX_ERROR(("Failure unpack agent report msg from %s, close it. err:%s", strConnIp.c_str(), tss->m_szBuf2K));
            strErr = "Failure unpack agent report msg, err=";
            strErr += tss->m_szBuf2K;
            bSuccess = false;
            break;
        }
        CWX_DEBUG(("Receive agent[%s]'s report from %s", szAgentIps?szAgentIps:"", strConnIp.c_str())); 
        strAgentIps = szAgentIps;
        CwxCommon::split(strAgentIps, ips, DCMD_ITEM_SPLIT_CHAR);
        if (!m_pApp->getTaskMgr()->getAgentIp(tss,
            ips,
            strAgentIp,
            strIpList,
            bRefresh))
        {
            CWX_ERROR(("report agent ip:%s isn't registered , close it.", szAgentIps));
            strErr = "report agent ip isn't registered";
            bSuccess = false;
            ///添加到无效的agent连接中
            m_pApp->getIllegalAgentMgr()->addConn(strConnIp, strAgentIps);
            break;
        }
        //鉴权
        string strOldConnIp;
        CWX_UINT32 uiOldConnId;
        int ret = m_pApp->getAgentMgr()->auth(msg->event().getConnId(),
            strAgentIp,
            szVer,
            strOldConnIp,
            uiOldConnId);
        if (2 == ret){///连接已经关闭
            CWX_ERROR(("Agent[%s] is closed.", strAgentIp.c_str()));
            return;
        }
        if (1 == ret){//auth ip exist，可能是非法冒充ip，也可能是重连
            if (strOldConnIp == strConnIp){//关闭旧连接
                m_pApp->getAgentMgr()->unAuth(strAgentIp);
                m_pApp->getTaskMgr()->agentClose(tss, strAgentIp);
                m_pApp->noticeCloseConn(uiOldConnId);
                ///重新认证
                ret = m_pApp->getAgentMgr()->auth(msg->event().getConnId(),
                    strAgentIp,
                    strOldConnIp,
                    uiOldConnId);
                CWX_INFO(("Connection for agent[%s] is duplicate, close the old", strAgentIp.c_str()));
                CWX_ASSERT(1 != ret); ///可能为2，连接不存在
            }else if(strConnIp==strAgentIp){///当前的连接就是正确的
                m_pApp->getAgentMgr()->unAuth(strAgentIp);
                m_pApp->getTaskMgr()->agentClose(tss, strAgentIp);
                m_pApp->noticeCloseConn(uiOldConnId);
                ///重新认证
                ret = m_pApp->getAgentMgr()->auth(msg->event().getConnId(),
                    strAgentIp,
                    strOldConnIp,
                    uiOldConnId);
                CWX_INFO(("Old conn[%s] for agent[%s] is invalid, close the old", strOldConnIp.c_str(), strAgentIp.c_str()));
                CWX_ASSERT(1 != ret); ///可能为2，连接不存在
                if (2 == ret) return; ///连接不存在
            }else{///通过数据库决策
                if (strIpList.find(strConnIp) == std::string::npos){
                    CWX_INFO(("conn[%s] for agent[%s] is invalid, close it", strConnIp.c_str(), strAgentIp.c_str()));
                    bSuccess = false;
                    strErr = "Failure to auth";
                    ///添加到无效的agent 连接中
                    CwxCommon::replaceAll(strAgentIps, ";", "");
                    m_pApp->getIllegalAgentMgr()->addConn(strConnIp, strAgentIps);
                    break;
                }
                m_pApp->getAgentMgr()->unAuth(strAgentIp);
                m_pApp->getTaskMgr()->agentClose(tss, strAgentIp);
                m_pApp->noticeCloseConn(uiOldConnId);
                CWX_INFO(("Old conn[%s] for agent[%s] is invalid, close the old", strOldConnIp.c_str(), strAgentIp.c_str()));
                ///重新认证
                ret = m_pApp->getAgentMgr()->auth(msg->event().getConnId(),
                    strAgentIp,
                    strOldConnIp,
                    uiOldConnId);
                CWX_ASSERT(1 != ret); ///可能为2，连接不存在
                if (2 == ret) return; ///连接不存在
            }
        }
        bSuccess = true;
    }while(0);

    if (bRefresh){
        m_pApp->getAgentMgr()->refreshAgent(m_pApp->getTaskMgr()->getIpTable());
    }

    ///回复
    replyAgentReport(this->m_pApp,
        tss,
        msg->event().getConnId(),
        msg->event().getMsgHeader().getTaskId(),
        bSuccess,
        m_pApp->getConfig().getCommon().m_uiHeatbeat,
        m_pApp->getConfig().getCommon().m_uiAgentPSize,
        strErr.c_str(),
        strAgentIp);
    ///如果自己是master，则通知agent
    if (bSuccess && m_pApp->isMaster()){
        noticeMaster(tss, &strAgentIp);
    }
    return;
}

///agent报告自己的任务处理状态
void  DcmdCenterH4AgentTask::agentMasterReply(CwxMsgBlock*& msg, DcmdTss* tss){
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