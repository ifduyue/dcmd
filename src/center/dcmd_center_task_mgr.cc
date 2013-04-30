#include "dcmd_center_task_mgr.h"

#include <CwxMd5.h>

#include "dcmd_center_app.h"
#include "dcmd_xml_parse.h"

dcmd {
DcmdCenterTaskMgr::DcmdCenterTaskMgr(DcmdCenterApp* app) {
  app_ = app;
  is_start_ = false;
  mysql_ = app->GetTaskMysql();
  next_task_id_ = 0;
  next_subtask_id_ = 0;
  next_cmd_id_ = 0;
  watches_ = NULL;
}

DcmdCenterTaskMgr::~DcmdCenterTaskMgr() {
  Reset();
}

bool DcmdCenterTaskMgr::Start(DcmdTss* tss) {
  // 若已经启动则直接退出
  if (is_start_) return true;
  // 若数据库无效则退出，随后的调用就不再检测数据库的有效性
  if (!app_->CheckMysql(mysql_)) return false;
  // 若当前不是master则直接退出
  if (!app_->is_master()) return false;
  // 设置启动状态
  is_start_ = true;
  // 加载数据
  if (!LoadAllDataFromDb(tss)) return false;
  return true;
}

void DcmdCenterTaskMgr::Stop(DcmdTss* ) {
  // 如果已经停止，则直接返回
  if (!is_start_) return;
  // 设置停止状态
  is_start_ = false;
  // 清空数据
  Reset();
}

bool DcmdCenterTaskMgr::ReceiveCmd(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd,
  uint32_t  conn_id,
  uint32_t  msg_task_id)
{
  dcmd_api::UiTaskCmdReply result;
  dcmd_api::DcmdState state;
  string master_host;
  // 若没有启动，则直接退出
  if (!is_start_ || !app_->is_master()) {
    if (!master_host.length() || master_host == app->config().common().host_id_) {
      state = dcmd_api::DCMD_STATE_NO_MASTER;
      strcpy(tss->m_szBuf2K, "No master.");
      tss->m_szBuf2K[0] = 0x00;
    } else {
      state = dcmd_api::DCMD_STATE_WRONG_MASTER;
      strcpy(tss->m_szBuf2K, master_host.c_str());
    }
  } if (!app_->CheckMysql(mysql_)) {
    state = dcmd_api::DCMD_STATE_FAILED;
    strcpy(tss->m_szBuf2K, "Can't connect to mysql.");
  } else {
    switch(cmd.cmd_type()) {
    case dcmd_api::CMD_START_TASK:
      state = TaskCmdStartTask(tss, cmd);
      break;
    case dcmd_api::CMD_PAUSE_TASK:
      state =TaskCmdPauseTask(tss, cmd);
      break;
    case dcmd_api::CMD_RESUME_TASK:
      state = TaskCmdResumeTask(tss, cmd);
      break;
    case dcmd_api::CMD_FINISH_TASK:
      state = TaskCmdFinishTask(tss, cmd);
      break;
    case dcmd_api::CMD_CANCEL_SUBTASK:
      state = TaskCmdCancelSubtask(tss, cmd);
      break;
    case dcmd_api::CMD_CANCEL_SVR_SUBTASK:
      state = TaskCmdCancelSvrSubtask(tss, cmd);
      break;
    case dcmd_api::CMD_REDO_TASK:
      state = TaskCmdRedoTask(tss, cmd);
      break;
    case dcmd_api::CMD_REDO_SVR_POOL:
      state = TaskCmdRedoSvrPool(tss, cmd);
      break;
    case dcmd_api::CMD_REDO_SUBTASK:
      state = TaskCmdRedoSubtask(tss, cmd);
      break;
    case dcmd_api::CMD_REDO_FAILED_SUBTASK:
      state = TaskCmdRedoFailedSubtask(task, cmd);
      break;
    case dcmd_api::CMD_REDO_FAILED_SVR_POOL_SUBTASK:
      state = TaskCmdRedoFailedSvrPoolSubtask(task, cmd);
      break;
    case dcmd_api::CMD_IGNORE_SUBTASK:
      state = TaskCmdIgnoreSubtask(task, cmd);
      break;
    case dcmd_api::CMD_FREEZE_TASK:
      state = TaskCmdFreezeTask(task, cmd);
      break;
    case dcmd_api::CMD_UNFREEZE_TASK:
      state = TaskCmdUnfreezeTask(task, cmd);
      break;
    case dcmd_api::CMD_UPDATE_TASK:
      state = TaskCmdUpdateTask(task, cmd);
      break;
    default:
      state = dcmd_api::DCMD_STATE_FAILED;
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Unknown task cmd type:%d", cmd.cmd_type());
    }
  }
  result.set_state(state);
  result.set_client_msg_id(cmd.client_msg_id());
  if (dcmd_api::DCMD_STATE_SUCCESS != state) {
    result.set_err(tss->m_szBuf2K);
  }
  DcmdCenterH4Admin::ReplyUiTaskCmd(this->app_, tss, conn_id, msg_task_id, &result);
}

bool DcmdCenterTaskMgr::ReceiveAgentMasterReply(DcmdTss* tss, ///<tss对象
  string const& agent_ip,
  list<string> const& agent_cmds
  )
{
  // 若没有启动，则直接退出
  if (!is_start_) return false;
  // 检查mysql是否正常，若不正常直接退出
  if (!app_->CheckMysql(mysql_)) return false;
  // 若不是master则直接退出
  if (!app_->is_master()) return false;
  // 发送agent上的所有信息
  set<uint64_t> cmd_set; ///存放所有agent上缓存的命令ID，防止再一次发送
  {
    list<string>::const_iterator iter = agent_cmds.begin();
    while (iter != agent_cmds.end()) {
      cmd_set.insert(strtoull(iter->c_str(), NULL, 10));
      ++iter;
    }
  }
  DcmdCenterAgent* agent = GetAgent(agent_ip);
  if (agent){
    // 如果不存在节点，则不作任何处理，否则将所有的命令一次性发送到agent
    map<uint64_t, DcmdCenterCmd*>::const_iterator iter = agent->cmds_.begin();
    DcmdCenterCmd
    while(iter != agent->cmds_.end()) {
      if (cmd_set.find(iter->first) == cmd_set.end()){
        // 指令没有分发
        if (!DcmdCenterH4AgentTask::SendAgentCmd(app_,
          tss,
          agent_ip,
          0,
          iter->second))
        {
          // 如果发送失败，只有连接已经关闭一种原因等待下一次连接的master-report
          return true;
        }
      }
      ++iter;
    }
  }
  return true;
}

bool DcmdCenterTaskMgr::ReceiveAgentClosed(DcmdTss* tss, string const& agent_ip) {
  // 若没有启动，则直接退出
  if (!is_start_) return false;
  // 检查mysql是否正常，若不正常直接退出
  if (!app_->CheckMysql(mysql_)) return false;
  // 若不是master则直接退出
  if (!app_->is_master()) return false;
  ///当前什么也无需做
  return true; ///<无需处理
}

bool DcmdCenterTaskMgr::ReceiveUiClosed(DcmdTss* tss, CWX_UINT32 conn_id) {
  // 删除UI连接上的watch
  if (watches_) watches_->CancelWatch(conn_id);
}


bool DcmdCenterTaskMgr::ReceiveAgentSubtaskConfirm(DcmdTss* tss,
  string const& agent_ip,
  string cmd_id)
{
  // 什么也不需要做
  return true;
}

bool DcmdCenterTaskMgr::ReceiveAgentSubtaskResult(DcmdTss* tss,
  uint32_t msg_task_id,
  dcmd_api::AgentTaskResult const& result)
{
  // 若没有启动，则直接退出
  if (!is_start_) return false;
  // 检查mysql是否正常，若不正常直接退出
  if (!app_->CheckMysql(mysql_)) return false;
  // 若不是master则直接退出
  if (!app_->is_master()) return false;
  DcmdCenterTask* task = NULL;
  string agent_ip;
  // 通知命令执行完毕
  if (!FinishTaskCmd(tss, result, agent_ip, task)) return false;
  //回复命令
  dcmd_api::AgentTaskResultReply  reply;
  reply.set_cmd(result.cmd());
  DcmdCenterH4AgentTask::ReplyAgentCmdResult(app_,
    tss,
    agent_ip,
    msg_task_id,
    &reply);
  //调度此任务的其他子任务任务
  if (!task) return true;
  return Schedule(tss, task);
}

void DcmdCenterTaskMgr::SetAgentTaskProcess(string const& task_id,
  string const& subtask_id,
  char const* process)
{
  CwxMutexGuard<CwxMutexLock> lock(&lock_);
  uint64_t subtask_id = strtoull(subtask_id.c_str(), NULL, 10);
  map<uint64_t, DcmdCenterSubtask*>::iterator iter = all_subtasks_.find(subtask_id);
  if (iter != all_subtasks_.end()){
    iter->second->set_process(process);
  }
}

bool DcmdCenterTaskMgr::GetAgentsTaskProcess(string const& subtask_id, string& process)
{
  CwxMutexGuard<CwxMutexLock> lock(&lock_);
  uint64_t subtask_id = strtoull(subtask_id.c_str(), NULL, 10);
  map<uint64_t, DcmdCenterSubtask*>::iterator iter = all_subtasks_.find(subtask_id);
  if (iter != all_subtasks_.end()){
    process = iter->second->process();
    return true;
  }
  return false;
}

void DcmdCenterTaskMgr::Reset() {
  is_start_ = false;
  { // 清空节点信息
    map<string, DcmdCenterAgent*>::iterator iter = agents_.begin();
    while(iter != agents_.end()) {
      delete iter->second;
      ++ iter;
    }
    agents_.clear();
  }
  { //清空所有任务
    map<uint32_t, DcmdCenterTask*>::iterator iter = all_tasks_.begin();
    while(iter != all_tasks_.end()){
      delete iter->second;
      ++iter;
    }
    all_tasks_.clear();
  }
  { // 清空所有子任务
    CwxMutexGuard<CwxMutexLock> lock(&m_lock);
    map<uint64_t, DcmdCenterSubtask*>::iterator iter = all_subtasks_.begin();
    while(iter != all_subtasks_.end()) {
      delete iter->second;
      ++iter;
    }
    all_subtasks_.clear();
  }
  { // 清空所有的命令
    map<uint64_t, DcmdCenterCmd*>::iterator iter = waiting_cmds_.begin();
    while(iter != waiting_cmds_.begin()){
      delete iter->second;
      ++iter;
    }
    waiting_cmds_.clear();
  }
  next_subtask_id_ = 0;
  next_subtask_id_ = 0;
  next_cmd_id_ = 0;
  // 关闭watch
  if (watches_) {
    watches_->NoticeMasterChange(true);
    delete watches_;
    watches_ = NULL;
  }
}

bool DcmdCenterTaskMgr::LoadAllDataFromDb(DcmdTss* tss) {
  // 加载所有的任务数据

  // 加载所有的subtask数据
  // 加载所有未处理的命令数据
}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdStartTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdPauseTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdResumeTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdFinishTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdCancelSubtask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdCancelSvrSubtask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoSvrPool(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoSubtask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoFailedSubtask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoFailedSvrPoolSubtask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdIgnoreSubtask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdFreezeTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdUnfreezeTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdUpdateTask(DcmdTss* tss,
  dcmd_api::UiTaskCmd const& cmd)
{

}

// 从数据库中获取新task
bool DcmdCenterTaskMgr::LoadNewTask() {
  CwxCommon::snprintf(tss)
}

}  // dcmd
