#include "dcmd_center_run_opr_task.h"

#include <CwxMd5.h>

#include "dcmd_center_app.h"
namespace dcmd {
void DcmdCenterOprTask::noticeTimeout(CwxTss* ) {
  is_reply_timeout_ = true;
  setTaskState(CwxTaskBoardTask::TASK_STATE_FINISH);
  CWX_DEBUG(("Task is timeout , task_id=%u", getTaskId()));
}

void DcmdCenterOprTask::noticeRecvMsg(CwxMsgBlock*& msg, CwxTss* ThrEnv, bool& ){
  DcmdTss* tss = (DcmdTss*)ThrEnv;
  for (uint16_t i=0; i<agent_num_; i++) {
    if (msg->event().getConnId() == agent_conns_[i].conn_id_) {
      agent_replys_[i].recv_msg_ = msg;
      msg = NULL;
      receive_num_++;
      if (receive_num_ >= agent_num_)
        setTaskState(CwxTaskBoardTask::TASK_STATE_FINISH);
      return ;
    }
  }
  CWX_ASSERT(0);
}

void DcmdCenterOprTask::noticeFailSendMsg(CwxMsgBlock*& msg, CwxTss* ) {
  for(uint32_t i=0; i<agent_num_; i++) {
    if (agent_conns_[i].conn_id_ == msg->send_ctrl().getConnId()) {
      receive_num_++;
      agent_replys_[i].is_send_failed_ = true;
      if (receive_num_ >= agent_num_)
        setTaskState(CwxTaskBoardTask::TASK_STATE_FINISH);
      return;
    }
  }
  CWX_ASSERT(0);
}

void DcmdCenterOprTask::noticeEndSendMsg(CwxMsgBlock*& , CwxTss* , bool& ){
}

void DcmdCenterOprTask::noticeConnClosed(CWX_UINT32 , CWX_UINT32 , CWX_UINT32 uiConnId, CwxTss*){
  for (uint16_t i=0; i<agent_num_; i++) {
    if (uiConnId == agent_conns_[i].conn_id_) {
      agent_replys_[i].recv_msg_ = NULL;
      agent_replys_[i].is_send_failed_ = true;
      receive_num_++;
      if (receive_num_ >= agent_num_) 
        setTaskState(CwxTaskBoardTask::TASK_STATE_FINISH);
      return;
    }
  }
}

bool DcmdCenterOprTask::FetchOprCmd(DcmdTss* tss){
  ///首先从cache中获取
  if (!m_pApp->getOprCmdCache()->getOprCmd(m_ullOprCmdId, m_oprCmd)){
    ///从db中获取
    Mysql* my = m_pApp->getAdminMysql();
    if (!m_pApp->checkMysql(my)){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to connect mysql, error=%s", my->getErrMsg());
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      return false;
    }
    //从mysql获取opr指令的信息
    CwxCommon::snprintf(tss->sql_,
      DcmdTss::kMaxSqlBufSize,
      "select a.ui_name, a.script_file, a.run_user,a.script_md5, b.timeout, b.ip, b.arg,"\
      "b.repeat, b.arg_mutable, b.cache from  opr_cmd as a, opr_cmd_exec as b"\
      "where b.id =%s and a.script_file = b.script_file",
      CwxCommon::toString(m_ullOprCmdId, tss->m_szBuf2K, 10));
    if (!my->query(tss->sql_)){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to fetch opr cmd from mysql, Sql:%s error=%s", tss->sql_, my->getErrMsg());
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      return false;
    }
    ///获取sql的结果
    if (1 != my->next()){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047,
        "Command[%s] doesn't exist in opr_cmd_exec table",
        CwxCommon::toString(m_ullOprCmdId, tss->sql_, 10));
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      ///释放结果集
      my->freeResult();
      return false;
    }
    bool bNull = false;
    char const* value = NULL;
    ///获取ui name
    m_oprCmd.m_strUiName = my->fetch(0, bNull);
    ///获取opr file
    m_oprCmd.m_strOprFile = my->fetch(1, bNull);
    ///获取run user
    m_oprCmd.m_strOprRunUser = my->fetch(2, bNull);
    ///获取opr file的md5
    m_oprCmd.m_strOprFileMd5 = my->fetch(3, bNull);
    ///获取opr的timeout
    value = my->fetch(4, bNull);
    m_oprCmd.m_uiOprTimeout = value?strtoul(value, NULL, 10):DCMD_DEF_OPR_CMD_TIMEOUT;
    if (m_oprCmd.m_uiOprTimeout < DCMD_MIN_OPR_CMD_TIMEOUT) m_oprCmd.m_uiOprTimeout = DCMD_MIN_OPR_CMD_TIMEOUT;
    if (m_oprCmd.m_uiOprTimeout > DCMD_MAX_OPR_CMD_TIMEOUT) m_oprCmd.m_uiOprTimeout = DCMD_MAX_OPR_CMD_TIMEOUT;
    ///获取ip
    m_oprCmd.m_strOprAgents = my->fetch(5, bNull);
    ///获取arg
    m_oprCmd.m_strArgs = my->fetch(6, bNull);
    ///获取repeat
    value  = my->fetch(7, bNull);
    m_oprCmd.m_ucRepeatType = value?strtoul(value, NULL, 10):DcmdCenterOprCmd::DCMD_OPR_CMD_NO_REPEAT;
    if (m_oprCmd.m_ucRepeatType > DcmdCenterOprCmd::DCMD_OPR_CMD_REPEAT_MAX) m_oprCmd.m_ucRepeatType = DcmdCenterOprCmd::DCMD_OPR_CMD_NO_REPEAT;
    if (m_pApp->getConfig().getCommon().m_bOprCmdExecHistory){
      if (m_oprCmd.m_ucRepeatType == DcmdCenterOprCmd::DCMD_OPR_CMD_REPEAT_WITHOUT_HISTORY)
        m_oprCmd.m_ucRepeatType = DcmdCenterOprCmd::DCMD_OPR_CMD_REPEAT_HISTORY;
    }
    ///获取arg mutable
    value = my->fetch(8, bNull);
    m_oprCmd.m_bArgMutable = value?(strtoul(value, NULL, 0)?true:false):false;
    if (!m_pApp->getConfig().getCommon().m_bOprCmdArgMutable) m_oprCmd.m_bArgMutable = false;
    ///获取cache
    value = my->fetch(9, bNull);
    m_oprCmd.m_uiCacheSec = value?strtoul(value, NULL,0):0;
    m_oprCmd.m_uiExpire = m_oprCmd.m_uiCacheSec?m_oprCmd->m_uiExpire = ((CWX_UINT32)time(NULL)) + m_oprCmd->m_uiCacheSec;
    ///释放结果集
    my->freeResult();
    ///获取脚本、检查md5
    string strOprFile;
    DcmdCenterConf::getOprCmdFile(m_oprCmd.m_strOprFile, strOprFile);
    strOprFile = m_pApp->getConfig().getCommon().m_strOprScriptPath + strOprFile;
    if (!tss->readFile(strOprFile.c_str(), m_oprCmd.m_strContent, m_strErrMsg)){
      CWX_ERROR((m_strErrMsg.c_str()));
      return false;
    }
    ///计算md5
    string strMd5;
    dcmdMd5(m_oprCmd.m_strContent.c_str(), m_oprCmd.m_strContent.length(), strMd5);
    if (strcasecmp(m_oprCmd.m_strOprFileMd5.c_str(), strMd5.c_str()) != 0){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "opr-file[%s]'s md5[%s] is not same with table's md5 is:%s",
        strOprFile.c_str(),
        strMd5.c_str(),
        m_oprCmd.m_strOprFileMd5.c_str());
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      return false;
    }
    ///解析ip参数
    list<string> ips;
    m_oprCmd.m_agentSet.clear();
    CwxCommon::split(m_oprCmd.m_strOprAgents, ips, DCMD_ITEM_SPLIT_CHAR);
    list<string>::iterator ip_iter = ips.begin();
    string strIp;
    while(ip_iter != ips.end()){
      strIp = *ip_iter;
      CwxCommon::trim(strIp);
      if (strIp.length()) m_agentSet.insert(strIp);
      ip_iter ++;
    }
    if (0 == m_agentSet.size()){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "No host in host list:%s", m_strOprAgents.c_str());
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      return false;
    }
    m_oprCmd.m_argMap.clear();
  }
  ///替换执行的参数
  if (m_bWithOprArg){
    if (!m_oprCmd.m_bArgMutable){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Can't change opr cmd's arg, opr cmd:%s",
        CwxCommon::toString(m_ullOprCmdId, tss->sql_, 10));
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      return false;
    }
    m_oprCmd.m_strArgs = m_strOprArg;
    m_oprCmd.m_argMap.clear();
  }
  ///形成参数
  if (m_oprCmd.m_strArgs.length() && !m_oprCmd.m_argMap.size()){
    XmlConfigParser parser;
    if (!parser.parse(m_oprCmd.m_strArgs.c_str())){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to parse shell arg for invalid xml, arg:%s", m_strArgs.c_str());
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      return false;
    }
    XmlTreeNode const* node = parser.getRoot()->m_pChildHead;
    string strNodeValue;
    while(node){
      if (node->m_pChildHead){
        CwxCommon::snprintf(tss->m_szBuf2K, 2047, "arg[%s] has child, it's invalid.", node->m_szElement);
        CWX_ERROR((tss->m_szBuf2K));
        m_strErrMsg = tss->m_szBuf2K;
        return false;
      }
      strNodeValue = "";
      list<char*>::const_iterator iter = node->m_listData.begin();
      while(iter != node->m_listData.end()){
        strNodeValue += *iter;
        iter++;
      }
      m_oprCmd.m_argMap.m_argMap[string(node->m_szElement)] = strNodeValue;
      node = node->m_next;
    }
  }

  ///形成历史
  bool bExecSql = false;
  if (m_oprCmd.m_ucRepeatType != DcmdCenterOprCmd::DCMD_OPR_CMD_REPEAT_WITHOUT_HISTORY){
    string strArg = m_oprCmd.m_strArgs;
    dcmdEscapeMysqlString(strArg);
    if (strArg.length() + 2048 > DcmdTss::kMaxSqlBufSize ){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "arg is too long, max:%d", kMaxSqlBufSize - 2048);
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      return false;

    }
    bExecSql = true;
    ///记录操作历史
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "insert into opr_cmd_exec_history(id, script_file, timeout, ip, repeat, arg_mutable, cache, arg, opr_user, create_time, exec_time) \
      select id, script_file, timeout, ip, repeat, arg_mutable, cache, '%s', opr_user, create_time, now() from opr_cmd_exec \
      where id=%s",
      strArg.c_str(),
      CwxCommon::toString(m_ullOprCmdId, tss->m_szBuf2K, 10));
    if (-1 == my->execute(tss->sql_)){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to exec sql:%s",  tss->sql_);
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      my->rollback();
      return false;
    }
  }
  ///删除数据
  if (DcmdCenterOprCmd::DCMD_OPR_CMD_NO_REPEAT == m_oprCmd.m_ucRepeatType){
    bExecSql = true;
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "delete from opr_cmd_exec where id=%s",
      CwxCommon::toString(m_ullOprCmdId, tss->m_szBuf2K, 10));
    if (-1 == my->execute(tss->sql_)){
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to delete opr , exec sql:%s",  tss->sql_);
      CWX_ERROR((tss->m_szBuf2K));
      m_strErrMsg = tss->m_szBuf2K;
      my->rollback();
      return false;
    }
  }
  if (bExecSql) my->commit();
  ///cache对象
  if (m_oprCmd.m_uiCacheSec){
    m_pApp->getOprCmdCache()->addOprCmd(m_ullOprCmdId, m_oprCmd);
  }
  return true;
}


int DcmdCenterOprTask::noticeActive(CwxTss* ThrEnv) {
  DcmdTss* tss= (DcmdTss*)ThrEnv;
  setTaskState(TASK_STATE_WAITING);
  if (!FetchOprCmd(tss)){
    setTaskState(TASK_STATE_FINISH);
    m_bFail = true;
    return -1;
  }
  CWX_UINT64 timeStamp = m_uiOprTimeout;
  timeStamp *= 1000000;
  timeStamp += CwxDate::getTimestamp();
  this->setTimeoutValue(timeStamp);

  CwxMsgBlock* msg=NULL;
  if (DCMD_ERR_SUCCESS != DcmdPoco::packAgentOprCmd(tss->m_pWriter,
    msg,
    getTaskId(),
    m_oprCmd.m_strUiName.c_str(),
    m_oprCmd.m_strOprFile.c_str(),
    m_oprCmd.m_strOprRunUser.c_str(),
    m_oprCmd.m_strContent.c_str(),
    m_oprCmd.m_strContent.length(),
    m_oprCmd.m_uiOprTimeout,
    m_oprCmd.m_argMap,
    tss->m_szBuf2K))
  {
    m_strErrMsg = "Failure to package shell package, err:";
    m_strErrMsg += tss->m_szBuf2K;
    CWX_ERROR((m_strErrMsg.c_str()));
    setTaskState(TASK_STATE_FINISH);
    m_bFail = true;
    return -1;
  }
  CwxMsgBlock* sendBlock = NULL;

  m_unAgentNum = m_oprCmd.m_agentSet.size();
  m_unRecvNum = 0;
  m_agentConns = new DcmdAgentConnect[m_unAgentNum];
  m_agentReply = new DcmdCenterAgentOprReply[m_unAgentNum];
  ///发送msg
  set<string>::iterator iter = m_oprCmd.m_agentSet.begin();
  CWX_UINT32 uiConnId = 0;
  CWX_UINT32 unIndex = 0;
  while(iter != m_oprCmd.m_agentSet.end()){
    if (!sendBlock) sendBlock = CwxMsgBlockAlloc::clone(msg);
    if (!DcmdCenterH4AgentOpr::SendAgentMsg(m_pApp,
      *iter,
      getTaskId(),
      sendBlock,
      uiConnId,
      true))
    {
      CWX_ERROR(("Failure send msg to agent:%s for opr:%s", iter->c_str(), m_oprCmd.m_strUiName.c_str()));
      m_agentConns[unIndex].m_uiConnId = 0;
      m_agentReply[unIndex].m_bSendFail = true;
      m_unRecvNum++;
    }else{
      sendBlock = NULL;
      m_agentConns[unIndex].m_uiConnId = uiConnId;
    }
    m_agentConns[unIndex].m_strAgentIp = *iter;
    unIndex++;
    iter++;
  }
  CwxMsgBlockAlloc::free(msg);
  if (sendBlock) CwxMsgBlockAlloc::free(sendBlock);
  if (m_unRecvNum == m_unAgentNum){
    setTaskState(TASK_STATE_FINISH);
  }
  return 0;
}

void DcmdCenterOprTask::execute(CwxTss* pThrEnv) {
  if (CwxTaskBoardTask::TASK_STATE_INIT == getTaskState())    {
    is_reply_timeout_ = false;
    is_failed_ = false;
    agent_num_ = 0;
    receive_num_ = 0;
    agent_conns_ = NULL;
    agent_replys_ = NULL;
    getTaskBoard()->noticeActiveTask(this, pThrEnv);
  }
  if (CwxTaskBoardTask::TASK_STATE_FINISH == getTaskState()) {
    Reply(pThrEnv);
    delete this;
  }
}

void DcmdCenterOprTask::reply(CwxTss* pThrEnv) {
  static string strSplit = "----------------------------------------------\n";
  DcmdTss* pTss = (DcmdTss*)pThrEnv;
  int ret = DCMD_ERR_SUCCESS;
  if (m_bFail) ret = DCMD_ERR_ERROR;
  CWX_UINT16 unIndex = 0;
  string strReply;
  CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "state:%s\nerr:%s\n",
    m_bFail?"error":"success",
    m_bFail?m_strErrMsg.c_str():"");
  strReply = pTss->m_szBuf2K;

  if (!m_bFail){
    ///输出发送失败的host
    for (unIndex =0; unIndex < m_unAgentNum; unIndex++){
      if (m_agentReply[unIndex].m_bSendFail){
        strReply += strSplit;
        sprintf(pTss->m_szBuf2K, "ip:%s\nstate:lost\nresult:\nlost connect\n",
          m_agentConns[unIndex].m_strAgentIp.c_str());
        strReply += pTss->m_szBuf2K;
      }
    }
    ///输出error的host
    for (unIndex =0; unIndex < m_unAgentNum; unIndex++){
      if (!m_agentReply[unIndex].m_bSendFail && !m_agentReply[unIndex].m_bExecSuccess){
        strReply += strSplit;
        CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "ip:%s\nstate:error\nresult:\n%s\n",
          m_agentConns[unIndex].m_strAgentIp.c_str(),
          m_agentReply[unIndex].m_szErrMsg);
        strReply += pTss->m_szBuf2K;
      }
    }
    ///输出timeout的
    for (unIndex =0; unIndex < m_unAgentNum; unIndex++){
      if (!m_agentReply[unIndex].m_bSendFail && !m_agentReply[unIndex].m_msg){
        strReply += strSplit;
        CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "ip:%s\nstate:timeout\nresult:\ntimeout\n",
          m_agentConns[unIndex].m_strAgentIp.c_str());
        strReply += pTss->m_szBuf2K;
      }
    }
    ///输出正常返回的
    for (unIndex =0; unIndex < m_unAgentNum; unIndex++){
      if (!m_agentReply[unIndex].m_bSendFail && m_agentReply[unIndex].m_msg && m_agentReply[unIndex].m_bExecSuccess){
        strReply += strSplit;
        CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "ip:%s\nstate=success\ndetail:\n",
          m_agentConns[unIndex].m_strAgentIp.c_str());
        strReply += pTss->m_szBuf2K;
        strReply += m_agentReply[unIndex].m_szResult;
        strReply += "\n";
      }
    }
  }
  //reply
  CWX_UINT32 uiState = m_bFail?DCMD_ERR_ERROR:DCMD_ERR_SUCCESS;
  DcmdCenterH4Admin::replyOprCmdResult(m_pApp,
    pTss,
    m_uiReplyConnId,
    m_uiMsgTaskId,
    uiState,
    strReply.c_str(),
    m_strErrMsg.c_str());
}
}  // dcmd