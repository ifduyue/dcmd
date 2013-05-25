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
      state = TaskCmdStartTask(tss, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_PAUSE_TASK:
      state =TaskCmdPauseTask(tss, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_RESUME_TASK:
      state = TaskCmdResumeTask(tss, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_RETRY_TASK:
      state = TaskCmdRetryTask(tss, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_FINISH_TASK:
      state = TaskCmdFinishTask(tss, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_CANCEL_SUBTASK:
      state = TaskCmdCancelSubtask(tss, strtoull(cmd.subtask_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_CANCEL_SVR_SUBTASK:
      state = TaskCmdCancelSvrSubtask(tss, cmd.svr_name().c_str(), cmd.ip().c_str(),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_REDO_TASK:
      state = TaskCmdRedoTask(tss, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_REDO_SVR_POOL:
      state = TaskCmdRedoSvrPool(tss, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.svr_pool().c_str(), cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_REDO_SUBTASK:
      state = TaskCmdRedoSubtask(tss, strtoull(cmd.subtask_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_REDO_FAILED_SUBTASK:
      state = TaskCmdRedoFailedSubtask(task, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_REDO_FAILED_SVR_POOL_SUBTASK:
      state = TaskCmdRedoFailedSvrPoolSubtask(task, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.svr_pool().c_str(), cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_IGNORE_SUBTASK:
      state = TaskCmdIgnoreSubtask(task, strtoull(cmd.subtask_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_FREEZE_TASK:
      state = TaskCmdFreezeTask(task, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_UNFREEZE_TASK:
      state = TaskCmdUnfreezeTask(task, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.uid(), NULL);
      break;
    case dcmd_api::CMD_UPDATE_TASK:
      state = TaskCmdUpdateTask(task, strtoul(cmd.task_id().c_str(), NULL, 10),
        cmd.concurrent_num(), cmd.concurrent_rate(), cmd.task_timeout(),
        cmd.auto_(), cmd.uid(), NULL);
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
  next_task_id_ = 0;
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
  // 加载所有新的任务数据
  if (!LoadNewTask(tss, true)) return false;
  // 加载所有的subtask数据
  if (!LoadNewSubtask(tss)) return false;
  // 加载所有未处理的命令数据
  if (!LoadAllCmd(tss)) return false;
  return true;
}


// 从数据库中获取新task
bool DcmdCenterTaskMgr::LoadNewTask(DcmdTss* tss, bool is_first) {
  list<DcmdCenterTask*> tasks;
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, "select task_id, task_name, task_cmd, parent_task_id,"\
    "order, svr_id, service, gid, group_name, tag, update_env, update_tag, state,"\
    "freeze, valid, pause, concurrent_num, concurrent_rate, timeout, auto, process, task_arg, "\
    "errmsg from task order by task_id asc where task_id > %d", next_task_id_);
  if (!mysql_->query(tss->sql_)){
    CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to fetch new tasks. err:%s; sql:%s", mysql_->getErrMsg(), tss->sql_);
    CWX_ERROR((tss->m_szBuf2K));
    mysql_->freeResult();
    return false;
  }
  bool is_null = false;
  DcmdCenterTask* task = NULL;
  while(mysql_->next()){
    task = new DcmdCenterTask();
    task->task_id_ = strtoul(mysql_->fetch(0, is_null), NULL, 10);
    task->task_name_ = mysql_->fetch(1, is_null);
    task->task_cmd_ = mysql_->fetch(2, is_null);
    task->parent_task_id_ = strtoul(mysql_->fetch(3, is_null), NULL, 10);
    task->doing_order_ = strtoul(mysql_->fetch(4, is_null), NULL, 10);
    task->svr_id_ = strtoul(mysql_->fetch(5, is_null), NULL, 10);
    task->service_ = mysql_->fetch(6, is_null);
    task->group_id_ = strtoul(mysql_->fetch(7, is_null), NULL, 10);
    task->group_name_ = mysql_->fetch(8, is_null);
    task->tag_ = mysql_->fetch(9, is_null);
    task->update_env_ = strtoul(mysql_->fetch(10, is_null), NULL, 10)?true:false;
    task->update_tag_ = strtoul(mysql_->fetch(11, is_null), NULL, 10)?true:false;
    task->state_ = strtoul(mysql_->fetch(12, is_null), NULL, 10);
    task->is_freezed_ = strtoul(mysql_->fetch(13, is_null), NULL, 10)?true:false;
    task->is_valid_ = strtoul(mysql_->fetch(14, is_null), NULL, 10)?true:false;
    task->is_pause_ = strtoul(mysql_->fetch(15, is_null), NULL, 10)?true:false;
    task->max_current_num_ = strtoul(mysql_->fetch(15, is_null), NULL, 10);
    task->max_current_rate_ = strtoul(mysql_->fetch(16, is_null), NULL, 10);
    task->timeout_ = strtoul(mysql_->fetch(17, is_null), NULL, 10);
    task->is_auto_ = strtoul(mysql_->fetch(18, is_null), NULL, 10)?true:false;
    task->is_output_process_ = strtoul(mysql_->fetch(19, is_null), NULL, 10)?true:false;
    task->arg_xml_ = mysql_->fetch(20, is_null);
    task->err_msg_ = mysql_->fetch(21, is_null);
    next_task_id_ = task->task_id_;
    // 将任务添加到任务列表中
    tasks.push_back(task);
    // 将任务添加到task的map中
    all_tasks_[task->task_id_] = task;
  }
  mysql_->freeResult();
  // 解析任务数据
  int ret = 0;
  list<DcmdCenterTask*>::iterator iter = tasks.begin();
  while( iter != tasks.end()) {
    task = *iter;
    // 无条件的加载task的任务池子
    if (!task->is_cluster_ && !LoadTaskSvrPool(tss, task)) return false;
    // 检查任务的各种信息
    if (task->is_valid_) {
      // 只有valid的task，才进行任务的parse处理，否则只能redo task的时候再处理
      if (!ParseTask(tss, task)) return false;
      // 检查任务的状态
      if (!is_first) {
        // 如果不是第一次加载，则需要child 任务的state必须为
        if (task->state_ != dcmd_api::TASK_INIT) {
          if (!UpdateTaskState(tss, true, task->task_id_, dcmd_api::TASK_INIT))
            return false;
          task->state_ = dcmd_api::TASK_INIT;
        }
      }
    }
    ++iter;
  }
  return true;
}

bool DcmdCenterTaskMgr::LoadNewSubtask(DcmdTss* tss) {
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
    "select subtask_id, task_id, task_cmd, svr_pool, service, ip, state, ignored,"\
    "UNIX_TIMESTAMP(start_time), UNIX_TIMESTAMP(finish_time), process, errmsg "\
    " from task_node where subtask_id >= %s order by subtask_id desc ",
    CwxCommon::toString(next_subtask_id_, tss->m_szBuf2K, 10));
  if (!mysql_->query(tss->sql_)) {
    mysql_->freeResult();
    CWX_ERROR(("Failure to fetch task-node. err:%s; sql:%s", mysql_->getErrMsg(),
      tss->sql_));
    return false;
  }
  bool is_null = false;
  DcmdCenterSubtask*  subtask = NULL;
  while(mysql_->next()){
    subtask = new DcmdCenterSubtask();
    subtask->subtask_id_ = strtoull(mysql_->fetch(0, is_null), NULL, 0);
    subtask->task_id_ = strtoull(mysql_->fetch(1, is_null), NULL, 0);
    subtask->task_cmd_ = mysql_->fetch(2, is_null);
    subtask->svr_pool_ = mysql_->fetch(3, is_null);
    subtask->service_ = mysql_->fetch(4, is_null);
    subtask->ip_ = mysql_->fetch(5, is_null);
    subtask->state_ = strtoul(mysql_->fetch(6, is_null), NULL, 0);
    subtask->is_ignored_ = strtoul(mysql_->fetch(7, is_null), NULL, 0)?true:false;
    subtask->start_time_ = strtoul(mysql_->fetch(8, is_null), NULL, 0);
    subtask->finish_time_ = strtoul(mysql_->fetch(9, is_null), NULL, 0);
    subtask->process_ = mysql_->fetch(10, is_null);
    subtask->err_msg_ = mysql_->fetch(11, is_null);
    next_subtask_id_ = subtask->subtask_id_;
    all_subtasks_[subtask->subtask_id_] = subtask;
  }
  mysql_->freeResult();
  // 将subtask加入到task中
  map<uint64_t, DcmdCenterSubtask*>::iterator iter = all_subtasks_.begin();
  while (iter != all_subtasks_.end()) {
    subtask = iter->second;
    subtask->task_ = GetTask(subtask->subtask_id_);
    if (!subtask->task_) {
      UpdateSubtaskState(tss, true, subtask->subtask_id_, dcmd_api::SUBTASK_FAILED, "No task.");
      all_subtasks_.erase(subtask->subtask_id_);
      iter = all_subtasks_.upper(subtask->subtask_id_);
      continue;
    }
    if (subtask->state_ > dcmd_api::SUBTASK_FAILED) {
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Invalid state[%d]", subtask->state_);
      subtask->state_ = dcmd_api::SUBTASK_FAILED;
      subtask->err_msg_ = tss->m_szBuf2K;
      UpdateSubtaskState(tss, true, subtask->subtask_id_, subtask->state_, tss->m_szBuf2K);
    }
    subtask->task_->AddSubtask(subtask);
    ++iter;
  }
  return true;
}

bool DcmdCenterTaskMgr::LoadAllCmd(DcmdTss* tss) {
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
    "select cmd_id, task_id, subtask_id, svr_pool, service, ip, state, cmd_type "\
    " from command where state = 0 order by cmd_id desc ");
  if (!mysql_->query(tss->sql_)) {
    mysql_->freeResult();
    CWX_ERROR(("Failure to fetch command. err:%s; sql:%s", mysql_->getErrMsg(),
      tss->sql_));
    return false;
  }
  bool is_null = false;
  DcmdCenterCmd*  cmd = NULL;
  next_cmd_id_ = 1;
  list<DcmdCenterCmd*> cmds;
  while(mysql_->next()){
    cmd = new DcmdCenterCmd();
    cmd->cmd_id_ = strtoull(mysql_->fetch(0, is_null), NULL, 0);
    cmd->task_id_ = strtoull(mysql_->fetch(1, is_null), NULL, 0);
    cmd->subtask_id_ = mysql_->fetch(2, is_null);
    cmd->svr_pool_ = mysql_->fetch(3, is_null);
    cmd->service_ = mysql_->fetch(4, is_null);
    cmd->agent_ip_ = mysql_->fetch(5, is_null);
    cmd->state_ = strtoul(mysql_->fetch(6, is_null), NULL, 0);
    cmd->cmd_type_ = strtoul(mysql_->fetch(7, is_null), NULL, 0);
    next_cmd_id_ = cmd->cmd_id_ + 1;
    cmds.push_back(cmd);
  }
  mysql_->freeResult();
  dcmd_api::DcmdState state;
  list<DcmdCenterCmd*>::iterator iter = cmds.begin();
  while (iter != cmds.end()) {
    cmd = iter->second;
    switch (cmd->cmd_type_) {
    case dcmd_api::CMD_START_TASK:
      state = TaskCmdStartTask(tss,  cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_PAUSE_TASK:
      state =TaskCmdPauseTask(tss, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_RESUME_TASK:
      state = TaskCmdResumeTask(tss, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_RETRY_TASK:
      state = TaskCmdRetryTask(tss, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_FINISH_TASK:
      state = TaskCmdFinishTask(tss, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_CANCEL_SUBTASK:
      state = TaskCmdCancelSubtask(tss, cmd->subtask_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_CANCEL_SVR_SUBTASK:
      state = TaskCmdCancelSvrSubtask(tss, cmd->service_.c_str(),
        cmd->agent_ip_.c_str(), 0, &cmd);
      break;
    case dcmd_api::CMD_REDO_TASK:
      state = TaskCmdRedoTask(tss, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_REDO_SVR_POOL:
      state = TaskCmdRedoSvrPool(tss, cmd->task_id_, cmd->svr_pool_.c_str(),
        0, &cmd);
      break;
    case dcmd_api::CMD_REDO_SUBTASK:
      state = TaskCmdRedoSubtask(tss, cmd->subtask_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_REDO_FAILED_SUBTASK:
      state = TaskCmdRedoFailedSubtask(task, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_REDO_FAILED_SVR_POOL_SUBTASK:
      state = TaskCmdRedoFailedSvrPoolSubtask(task, cmd->task_id_,
        cmd->svr_pool_.c_str(), 0, &cmd);
      break;
    case dcmd_api::CMD_IGNORE_SUBTASK:
      state = TaskCmdIgnoreSubtask(task, cmd->subtask_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_FREEZE_TASK:
      state = TaskCmdFreezeTask(task, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_UNFREEZE_TASK:
      state = TaskCmdUnfreezeTask(task, cmd->task_id_, 0, &cmd);
      break;
    case dcmd_api::CMD_UPDATE_TASK:
      state = dcmd_api::DCMD_STATE_SUCCESS;
      break;
    default:
      state = dcmd_api::DCMD_STATE_FAILED;
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Unknown task cmd type:%d", cmd->cmd_type_);
      break;
    }
    if (cmd) delete cmd;
    if (!mysql_->IsConnected) break;
    if (state != dcmd_api::DCMD_STATE_SUCCESS) {
      if (!UpdateCmdState(tss, true, cmd->cmd_id_, dcmd_api::COMMAND_FAILED, tss->m_szBuf2K))
        break;
    } else {
      if (!UpdateCmdState(tss, true, cmd->cmd_id_, dcmd_api::COMMAND_SUCCESS, "")) {
        break;
      }
    }
    ++iter;
  }
  if (iter != cmds->end()) {
    ++iter;
    while(iter != cmds.end()) {
      delete *iter;
      ++iter;
    }
    return false;
  }
  return true;
}

bool DcmdCenterTaskMgr::LoadTaskSvrPool(DcmdTss* tss, DcmdCenterTask* task) {
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
    "select svr_pool, svr_pool_id, env_ver, repo, run_user from task_service_pool where task_id=%u",
    task->task_id_);
  if (!mysql_->query(tss->sql_)) {
    mysql_->freeResult();
    CWX_ERROR(("Failure to fetch task[%u]'s service pool. err:%s; sql:%s", mysql_->getErrMsg(),
      tss->sql_));
    return false;
  }
  bool is_null = false;
  DcmdCenterSvrPool*  svr_pool = NULL;
  while(mysql_->next()){
    svr_pool = new DcmdCenterSvrPool(task->task_id_);
    svr_pool->task_cmd_ = task->task_cmd_;
    svr_pool->svr_pool_ = mysql_->fetch(0, is_null);
    svr_pool->svr_pool_id_ = strtoul(mysql_->fetch(1, is_null), NULL, 10);
    svr_pool->svr_env_ver_ = mysql_->fetch(2, is_null);
    svr_pool->repo_ = mysql_->fetch(3, is_null);
    svr_pool->run_user_ = mysql_->fetch(4, is_null);
    // 将service pool添加到任务中
    task->AddSvrPool(svr_pool);
  }
  mysql_->freeResult();
  return true;
}

bool DcmdCenterTaskMgr::ParseTask(DcmdTss* tss, DcmdCenterTask* task) {
  // 解析xml的参数
  task->args_.clear();
  bool old_valid_state = task->is_valid_;
  do {
    task->is_valid_ = true;
    task->err_msg_ = "";
    if (task->arg_xml_.length()) {
      XmlConfigParser xml;
      if (!xml.parse(task->arg_xml_.c_str())){
        task->is_valid_ = false;
        task->err_msg_ = "Failure to parse task's xml.";
        break;
      }
      XmlTreeNode const* xmlnode = xml.getRoot();
      if(xmlnode && xmlnode->m_pChildHead){
        xmlnode = xmlnode->m_pChildHead;
        string arg_name;
        string arg_value;
        list<char*>::const_iterator iter;
        while(xmlnode){
          arg_name = xmlnode->m_szElement;
          dcmd_remove_spam(arg_name);
          iter = xmlnode->m_listData.begin();
          arg_value = "";
          while(iter != xmlnode->m_listData.end()){
            arg_value += *iter;
            iter++;
          }
          dcmd_remove_spam(arg_value);
          task->args_[arg_name] = arg_value;                    
          xmlnode=xmlnode->m_next;
        }
      }
    }
    // load task_cmd文件
    string md5;
    int ret = FetchTaskCmdInfoFromDb(tss, task->task_cmd_.c_str(), md5, task->is_cluster_);
    if (-1 == ret) return false;
    if (0 == ret) {
      task->is_valid_ = false;
      task->err_msg_ = tss->m_szBuf2K;
      break;
    }
    if (task->is_cluster_) {
      if (task->parent_task_id_) { // cluster任务不应该有parent task id
        CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Task cmd[%s] is cluster task, parent_task_id should be zero.");
        task->is_valid_ = false;
        task->err_msg_ = tss->m_szBuf2K;
        break;
      }
      // cluster task没有task_cmd的script
      break;
    } else {
      if (task->task_id_ == task->parent_task_id_) {
        task->parent_task_id_ = 0;
        task->is_valid_ = false;
        task->err_msg_ = "Task's parent-task-id is self.";
        break;
      }
    }
    // load task_cmd文件
    if (!ReadTaskCmdContent(tss, task->task_cmd_.c_str(), task->task_cmd_script_)) {
      task->is_valid_ = false;
      task->err_msg_ = tss->m_szBuf2K;
      break;
    }
    // 形成文件的md5签名
    dcmd_md5(task->task_cmd_script_.c_str(), task->task_cmd_script_.length(),
      task->task_cmd_script_md5_);
    if (strcasecmp(md5, task->task_cmd_script_md5_.c_str()) != 0) {
      CwxCommon::snprintf(tss->m_szBuf2K, 2047, "task-cmd[%s]'s md5 is wrong,"\
        "task_cmd table's md5 is:%s, but file's md5:%s",
        md5.c_str(), task->task_cmd_script_md5_.c_str());
      task->is_valid_ = false;
      task->err_msg_ = tss->m_szBuf2K;
      break;
    }
    // 建立cluster的任务关系
    if (task->parent_task_id_) {
      if (task->parent_task_id_ == task->task_id_) {
        CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Task's parent[%u] is self", task->task_id_);
        task->is_valid_ = false;
        task->err_msg_ = tss->m_szBuf2K;
        break;
      } else if (task->is_cluster_) {
        task->is_valid_ = false;
        task->err_msg_ = "Cluster task can't be child task.";
        break;
      } else {
        task->parent_task_ = GetTask(task->parent_task_id_);
        if (!task->parent_task_) {
          task->is_valid_ = false;
          task->err_msg_ = "Parent task is missing.";
          break;
        } else {
          if (!task->parent_task_->is_cluster_) {
            CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Task's parent[%u] is not cluster task", task->parent_task_->task_id_);
            task->is_valid_ = false;
            task->err_msg_ = tss->m_szBuf2K;
            break;
          }
          // 更新cluster task的状态
          if (task->parent_task_->is_valid_) {
            if (!task->is_valid_) {
              task->parent_task_->is_valid_ = false;
              task->parent_task_->err_msg_ = task->err_msg_;
              if (!UpdateTaskValid(tss, true, task->parent_task_->task_id_, false, task->err_msg_.c_str()))
                return false;
            }
          }
          task->parent_task_->AddChildTask(task);
        }
      }
    }
  }while(0);
  if (!task->is_valid_ || (old_valid_state != task->is_valid_)){
    if (!UpdateTaskValid(tss, true, task->task_id_, false, task->err_msg_.c_str()))
      return false;
  }
  return true;
}

bool DcmdCenterTaskMgr::ReadTaskCmdContent(DcmdTss* tss, char const* task_cmd, string& content) {
  string script_file;
  script_file = DcmdCenterConf::task_cmd_file(task_cmd, script_file);
  script_file = app_->config().common().task_script_path_ + "/" + script_file;
  bool ret =  CwxFile::readTxtFile(script_file, content);
  if (!ret){
    CwxCommon::snprintf(tss->m_szBuf2K, 2047, 
      "Failure to read task-cmd[%s]'s script[%s], errno=%d",
      task_cmd, script_file.c_str(), errno);
    return false;
  }
  if (!content.length()){
    CwxCommon::snprintf(tss->m_szBuf2K, 2047, "task-cmd[%s]'s script is empty,script:%s",
      task_cmd, script_file.c_str());
    return false;
  }
  return true;
}

int DcmdCenterTaskMgr::FetchTaskCmdInfoFromDb(DcmdTss* tss, , char const* task_cmd,
  string& md5, bool& is_cluster) {
  string task_cmd_str = task_cmd;
  dcmd_escape_mysql_string(task_cmd_str);
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
    "select script_md5, cluster from task_cmd where task_cmd ='%s'", task_cmd_str.c_str());
  if (!mysql_->query(tss->sql_)) {
    CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to fetch task_cmd[%s]. err:%s; sql:%s",
      task_cmd, mysql_->getErrMsg(), tss->sql_);
    mysql_->freeResult();
    return -1;
  }
  bool is_null = true;
  while(mysql_->next()){
    md5 = mysql_->fetch(0, bNull);
    is_cluster = strtoul(mysql_->fetch(1, bNull))?true:false;
    mysql_->freeResult();
    return 1;
  }
  mysql_->freeResult();
  CwxCommon::snprintf(tss->m_szBuf2K, 2047, "task-cmd[%s] doesn't exist in task_cmd table",
    task_cmd);
  return 0;
}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdStartTask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd) {
    DcmdCenterTask* task = NULL;
    // 加载任务
    if (!LoadNewTask(tss)) {
      mysql_->disconnect();
      return dcmd_api::DCMD_STATE_FAILED;
    }
    task = GetTask(task_id);
    if (!task) {
      tss->err_msg_ = "No task.";
      return dcmd_api::DCMD_STATE_NO_TASK;
    }
    if (!task->is_valid_) {
      tss->err_msg_ = "Task is invalid.";
      return dcmd_api::DCMD_STATE_FAILED;
    }
    if (task->parent_task_) {
      tss->err_msg_ = "Can't start child task.";
      return dcmd_api::DCMD_STATE_FAILED;
    }
    if (dcmd_api::TASK_INIT != task->state_) {
      tss->err_msg_ = "Task is started.";
      return dcmd_api::DCMD_STATE_FAILED;
    }
    bool is_success = true;
    do {
      // 初始化任务
      if (task->is_cluster_) { // cluster任务
        map<uint32_t, map<uint32_t, DcmdCenterTask*> >::iterator iter = task->child_tasks_.begin();
        map<uint32_t, DcmdCenterTask*>::iterator level_iter;
        while(iter != task->child_tasks_.end()) {
          level_iter = iter->second->begin();
          while(level_iter != iter->second->end) {
            if (!CreateSubtasksForTask(tss, task, false, uid)) {
              mysql_->disconnect();
              is_success = false;
              break;
            }
            ++level_iter;
          }
          if (!is_success) break;
          ++iter;
        }
        if (!is_success) break;
      } else {
        if (!CreateSubtasksForTask(tss, task, false, uid)){
          mysql_->disconnect();
          is_success = false;
          break;
        }
      }
      if (!cmd) {
        // 插入start的命令
        if (!InsertCommand(tss, false, uid, task->task_id_,
          0, "", 0, task->service_.c_str(), "", dcmd_api::CMD_START_TASK, 
          dcmd_api::COMMAND_SUCCESS, ""))
        {
          mysql_->disconnect();
          is_success = false;
          break;
        }
      } else {
        if (!UpdateCmdState(tss, false, cmd->cmd_id_, dcmd_api::COMMAND_SUCCESS, "")) {
          mysql_->disconnect();
          is_success = false;
          break;
        }
      }
      // 更新任务状态
      if (!UpdateTaskState(tss, true, task->task_id_, dcmd_api::TASK_DOING)) {
        mysql_->disconnect();
        is_success = false;
        break;
      }
    } while(0);
    if (!is_success) return dcmd_api::DCMD_STATE_FAILED;
    task->state_ = dcmd_api::TASK_DOING;
    if (!LoadNewSubtask(tss)) {
      mysql_->disconnect();
      return dcmd_api::DCMD_STATE_FAILED;
    }
    return dcmd_api::DCMD_STATE_SUCCESS;
}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdPauseTask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{
  DcmdCenterTask* task =  GetTask(task_id);
  // 加载任务
  if (!task) {
    if (!LoadNewTask(tss)) {
      mysql_->disconnect();
      return dcmd_api::DCMD_STATE_FAILED;
    }
    task = GetTask(task_id);
  }
  if (!task) {
    tss->err_msg_ = "No task.";
    return dcmd_api::DCMD_STATE_NO_TASK;
  }
  if (task->parent_task_) {
    tss->err_msg_ = "Can't pause child task.";
    return dcmd_api::DCMD_STATE_FAILED;
  }
  if (task->is_pause_) return dcmd_api::DCMD_STATE_SUCCESS;
  bool is_success = true;
  do {
    if (!cmd) {
      // 插入start的命令
      if (!InsertCommand(tss, false, uid, task->task_id_,
        0, "", 0, task->service_.c_str(), "", dcmd_api::CMD_PAUSE_TASK,
        dcmd_api::COMMAND_SUCCESS, ""))
      {
        mysql_->disconnect();
        is_success = false;
        break;
      }
    } else {
      if (!UpdateCmdState(tss, false, cmd->cmd_id_, dcmd_api::COMMAND_SUCCESS, "")) {
        mysql_->disconnect();
        is_success = false;
        break;
      }
    }
    // 更新任务状态
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
      "update task set pause=1 where task_id = %u", task->task_id_);
    if (!ExecSql(tss, true)) {
      mysql_->disconnect();
      is_success = false;
      break;
    }
  }while(0);
  if (!is_success) return dcmd_api::DCMD_STATE_FAILED;
  task->is_pause_ = true;
  return dcmd_api::DCMD_STATE_SUCCESS;
}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdResumeTask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{
  DcmdCenterTask* task =  GetTask(task_id);
  // 加载任务
  if (!task) {
    if (!LoadNewTask(tss)) {
      mysql_->disconnect();
      return dcmd_api::DCMD_STATE_FAILED;
    }
    task = GetTask(task_id);
  }
  if (!task) {
    tss->err_msg_ = "No task.";
    return dcmd_api::DCMD_STATE_NO_TASK;
  }
  if (task->parent_task_) {
    tss->err_msg_ = "Can't resume child task.";
    return dcmd_api::DCMD_STATE_FAILED;
  }
  if (!task->is_pause_) return dcmd_api::DCMD_STATE_SUCCESS;
  bool is_success = true;
  do {
    if (!cmd) {
      // 插入start的命令
      if (!InsertCommand(tss, false, uid, task->task_id_,
        0, "", 0, task->service_.c_str(), "", dcmd_api::CMD_RESUME_TASK,
        dcmd_api::COMMAND_SUCCESS, ""))
      {
        mysql_->disconnect();
        is_success = false;
        break;
      }
    } else {
      if (!UpdateCmdState(tss, false, cmd->cmd_id_, dcmd_api::COMMAND_SUCCESS, "")) {
        mysql_->disconnect();
        is_success = false;
        break;
      }
    }
    // 更新任务状态
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
      "update task set pause=0 where task_id = %u", task->task_id_);
    if (!ExecSql(tss, true)) {
      mysql_->disconnect();
      is_success = false;
      break;
    }
  }while(0);
  if (!is_success) return dcmd_api::DCMD_STATE_FAILED;
  task->is_pause_ = false;
  return dcmd_api::DCMD_STATE_SUCCESS;
}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRetryTask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{
  DcmdCenterTask* task =  GetTask(task_id);
  // 加载任务
  if (!task) {
    if (!LoadNewTask(tss)) {
      mysql_->disconnect();
      return dcmd_api::DCMD_STATE_FAILED;
    }
    task = GetTask(task_id);
  }
  if (!task) {
    tss->err_msg_ = "No task.";
    return dcmd_api::DCMD_STATE_NO_TASK;
  }
  if (task->parent_task_) {
    tss->err_msg_ = "Can't retry child task.";
    return dcmd_api::DCMD_STATE_FAILED;
  }
  if (task->is_valid_) return dcmd_api::DCMD_STATE_SUCCESS;
  bool is_success = true;
  string err_msg;
  do {
    if (!ParseTask(tss, task)) return false;
    if (task->is_cluster_) {
      map<uint32_t, map<uint32_t, DcmdCenterTask*>* >::iterator iter = task->child_tasks_.begin();
      map<uint32_t, DcmdCenterTask*>::iterator level_iter;
      while(iter != task->child_tasks_.end()) {
        level_iter = iter->second->begin();
        while(level_iter != iter->second->end()) {
          if (!ParseTask(tss, level_iter->second)) return false;
          ++level_iter;
        }
        ++iter;
      }
    }
    if (!cmd) {
      // 插入start的命令
      if (!InsertCommand(tss, false, uid, task->task_id_,
        0, "", 0, task->service_.c_str(), "", dcmd_api::CMD_RETRY_TASK,
        dcmd_api::COMMAND_SUCCESS, ""))
      {
        mysql_->disconnect();
        is_success = false;
        break;
      }
    } else {
      if (!UpdateCmdState(tss, false, cmd->cmd_id_, dcmd_api::COMMAND_SUCCESS, "")) {
        mysql_->disconnect();
        is_success = false;
        break;
      }
    }
  }while(0);
  if (!task->is_valid_) return dcmd_api::DCMD_STATE_FAILED;
  return dcmd_api::DCMD_STATE_SUCCESS;
}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdFinishTask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{
  DcmdCenterTask* task =  GetTask(task_id);
  // 加载任务
  if (!task) {
    if (!LoadNewTask(tss)) {
      mysql_->disconnect();
      return dcmd_api::DCMD_STATE_FAILED;
    }
    task = GetTask(task_id);
  }
  if (!task) {
    tss->err_msg_ = "No task.";
    return dcmd_api::DCMD_STATE_NO_TASK;
  }
  if (task->parent_task_) {
    tss->err_msg_ = "Can't finish child task.";
    return dcmd_api::DCMD_STATE_FAILED;
  }
  if (!task->is_freezed_) {
    tss->err_msg_ = "Task is not in freezed state.";
    return dcmd_api::DCMD_STATE_FAILED;
  }
  // 获取task id
  string task_ids;
  if (task->is_cluster_) {
    CwxCommon::sprintf(tss->m_szBuf2K, "%u", task->task_id_);
    task_ids = tss->m_szBuf2K;
    map<uint32_t, map<uint32_t, DcmdCenterTask*>* >::iterator iter = task->child_tasks_.begin();
    map<uint32_t, DcmdCenterTask*>::iterator level_iter;
    while(iter != task->child_tasks_.end()) {
      level_iter = iter->second->begin();
      while( level_iter != iter->second->end()) {
        CwxCommon::sprintf(tss->m_szBuf2K, ",%u", level_iter->second->task_id_);
        task_ids += tss->m_szBuf2K;
        level_iter++;
      }
      ++iter;
    }
  } else {
    CwxCommon::sprintf(tss->m_szBuf2K, "%u", task->task_id_);
    task_ids = tss->m_szBuf2K;
  }
  bool is_success = false;
  do {
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "delete from task_history where task_id in (%s)", task_ids.c_str());
    if (-1 == mysql_->execute(tss->sql_)) break;
    
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "insert into task_history select * from task where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;
    
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "delete from task where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
      "delete from task_service_pool_history where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "insert into task_service_pool_history select * from  task_service_pool where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "delete from task_service_pool where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
      "delete from task_node_history where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "insert into task_node_history select * from task_node where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "delete from task_node where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;
    
    if (!cmd) {
      // 插入start的命令
      if (!InsertCommand(tss, false, uid, task->task_id_,
        0, "", 0, task->service_.c_str(), "", dcmd_api::CMD_RETRY_TASK,
        dcmd_api::COMMAND_SUCCESS, "")) break;
    } else {
      if (!UpdateCmdState(tss, false, cmd->cmd_id_, dcmd_api::COMMAND_SUCCESS, ""))
        break;
    }
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
      "delete from command_history where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "insert into command_history select * from command where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "delete from command where task_id in (%s)", task_ids.c_str());
    if (-1 == m_mysql->execute(tss->sql_)) break;

    is_success = true;
  }while(0);
  if (!is_success) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg() +
      string("; sql:") + tss->sql_;
    mysql_->rollback();
    mysql_->disconnect();
    return dcmd_api::DCMD_STATE_FAILED;
  }
  if (!mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit, err:") + mysql_->getErrMsg();
    mysql_->rollback();
    mysql_->disconnect();
    return dcmd_api::DCMD_STATE_FAILED;
  }
  // 将task从内存中删除
  RemoveTaskFromMem(task);
  return dcmd_api::DCMD_STATE_SUCCESS;
}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdCancelSubtask(DcmdTss* tss, uint64_t subtask_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{
  DcmdCenterSubtask* subtask =  GetSubTask(subtask_id);
  if (!subtask) {
    tss->err_msg_ = "No subtask.";
    return dcmd_api::DCMD_STATE_NO_SUBTASK;
  }
  if (!subtask->task_) {
    tss->err_msg_ = "No task";
    return dcmd_api::DCMD_STATE_NO_TASK;
  }
  if (!subtask->task_->is_freezed_) {
    tss->err_msg_ = "Task is not in freezed state.";
    return dcmd_api::DCMD_STATE_FAILED;
  }
  if (!subtask->task_->parent_task_) {
    if (!subtask->task_->state_ == dcmd_api::TASK_INIT) {
      tss->err_msg_ = "Task is not in freezed state.";
      return dcmd_api::DCMD_STATE_FAILED;
    }
  }

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdCancelSvrSubtask(DcmdTss* tss, char const* serivce, 
  char const* agent_ip, uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoTask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoSvrPool(DcmdTss* tss, uint32_t task_id,
  char const* svr_pool, uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoSubtask(DcmdTss* tss, uint64_t subtask_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoFailedSubtask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdRedoFailedSvrPoolSubtask(DcmdTss* tss, uint32_t task_id,
  char const* svr_pool, uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdIgnoreSubtask(DcmdTss* tss, uint64_t subtask_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdFreezeTask(DcmdTss* tss,  uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdUnfreezeTask(DcmdTss* tss, uint32_t task_id,
  uint32_t uid, DcmdCenterCmd** cmd)
{

}

dcmd_api::DcmdState DcmdCenterTaskMgr::TaskCmdUpdateTask(DcmdTss* tss, uint32_t task_id,
  uint32_t con_num, uint32_t con_rate, uint32_t timeout, bool is_auto,
  uint32_t uid, DcmdCenterCmd** cmd)
{

}

}  // dcmd
