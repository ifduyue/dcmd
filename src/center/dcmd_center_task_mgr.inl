﻿namespace dcmd {

inline DcmdCenterTask* DcmdCenterTaskMgr::GetTask(uint32_t task_id) {
  map<uint32_t, DcmdCenterTask*>::iterator iter = all_tasks_.find(task_id);
  if (iter == all_tasks_.end()) return NULL;
  return iter->second;
}

inline DcmdCenterSubtask* DcmdCenterTaskMgr::GetSubTask(uint64_t subtak_id) {
  map<uint64_t, DcmdCenterSubtask*>::iterator iter =  all_subtasks_.find(subtask_id);
  if (iter == all_subtasks_.end()) return NULL;
  return iter->second;
}

// 获取agent
inline DcmdCenterAgent* DcmdCenterTaskMgr::GetAgent(string const& agent_ip) {
  map<string, DcmdCenterAgent*>::iterator iter = agents_.find(agent_ip);
  if (iter == agents_.end()) return NULL;
  return iter->second;
}


inline bool DcmdCenterTaskMgr::IsTaskFreezed(DcmdCenterTask* task) {
  if (task->parent_task_) return task->parent_task_->is_freezed_;
  return task->is_freezed_;
}


inline bool DcmdCenterTaskMgr::UpdateTaskValid(DcmdTss* tss, bool is_commit, 
  uint32_t task_id, bool is_valid, char const* err_msg)
{
  string str_tmp = err_msg?string(err_msg):"";
  dcmd_escape_mysql_string(str_tmp);
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
    "update task set valid=%d, errmsg='%s where task_id=%d',
    is_valid?1:0, is_valid?"":str_tmp.c_str(), task_id);
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  return true;
}

inline bool DcmdCenterTaskMgr::UpdateTaskState(DcmdTss* tss, bool is_commit,
  uint32_t task_id, dcmd_api::TaskState state) {
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
      "update task set state=%d where task_id=%d', state);
    if (-1 == mysql_->execute(tss->sql_)) {
      tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
        + ". sql:" + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + ". sql:" + tss->sql_;
      CWX_ERROR((tss->err_msg.c_str()));
      mysql_->rollback();
      return false;
    }
    return true;
}

inline bool DcmdCenterTaskMgr::UpdateSubtaskState(DcmdTss* tss, bool is_commit,
  uint64_t subtask_id, uint8_t state, char const* err_msg)
{
  char buf[64];
  string str_tmp = string(err_msg);
  dcmd_escape_mysql_string(str_tmp);
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
    "update task_node set state=%d , errmsg = %s where subtask_id=%s",
    state， str_tmp.c_str(), CwxCommon::toString(subtask_id, buf, 10));
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg_.c_str()));
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  return true;
}

// 更新命令的状态
inline bool DcmdCenterTaskMgr::UpdateCmdState(DcmdTss* tss, bool is_commit,
  uint64_t cmd_id, uint8_t state, char const* err_msg)
{
  char buf[64];
  string str_tmp = string(err_msg);
  dcmd_escape_mysql_string(str_tmp);
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
    "update command set state=%d, errmsg = %s where cmd_id=%s",
    state， str_tmp.c_str(), CwxCommon::toString(subtask_id, buf, 10));
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg_.c_str()));
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  return true;
}

inline bool DcmdCenterTaskMgr::UpdateTaskInfo(DcmdTss* tss, bool is_commit,
  uint32_t task_id, uint32_t con_num, uint32_t con_rate, uint32_t timeout,
  bool is_auto, uint32_t uid)
{
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
    "update task set concurrent_num=%d, concurrent_rate=%d, timeout=%d, auto=%d,"\
    "opr_uid=%d, utime=now() where task_id=%u",
    con_num, con_rate, timeout, is_auto?1:0, uid, task_id);
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg_.c_str()));
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  return true;
}

// 创建任务的子任务
inline bool DcmdCenterTaskMgr::CreateSubtasksForTask(DcmdTss* tss, DcmdCenterTask* task,
  bool is_commit, uint32_t uid)
{
  // 删除旧任务
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
    "delete from task_node where task_id = %u", task->task_id_);
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg_.c_str()));
    mysql_->rollback();
    return false;
  }
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
    "insert into task_node(task_id, task_cmd, svr_pool, service, ip, state, ignored,"\
    "start_time, finished_time, process, errmsg, utime, ctime, opr_uid) "\
    "select t.task_id, t.task_cmd, p.svr_pool, t.service, n.ip, 0, 0, now(), now(), "", "", now(), now(), %u " \
    "from task as t, task_service_pool as p, service_pool_node as n "\
    "where t.task_id = %u and p.task_id=%u "\
    " and p.svr_pool_id = n.svr_pool_id",
    task->task_id_, task->task_id_);
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg_.c_str()));
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  return true;
}

inline bool DcmdCenterTaskMgr::InsertCommand(DcmdTss* tss, bool is_commit, uint32_t uid,
  uint32_t task_id, uint64_t subtask_id, char const* svr_pool,
  uint32_t svr_pool_id, char const* service, char const* ip,
  uint8_t cmt_type, uint8_t state, char const* err_msg)
{
  string svr_pool_str(svr_pool?str_pool:"");
  string service_str(service?service:"");
  string ip_str(ip?ip:"");
  string err_msg_str(err_msg?err_msg:"");
  dcmd_escape_mysql_string(svr_pool_str);
  dcmd_escape_mysql_string(service_str);
  dcmd_escape_mysql_string(ip_str);
  dcmd_escape_mysql_string(err_msg_str);
  char cmd_id_sz[65];
  char subtask_id_sz[65];
  CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
    "insert into command(cmd_id, task_id, subtask_id, svr_pool, svr_pool_id, service, ip,"\
    "cmd_type, state, errmsg, utime, ctime, opr_uid) "\
    " values (%s, %u, %s, '%s', %u, '%s', %u, %u, '%s', now(), now(), %u)",
    CwxCommon::snprintf(++next_cmd_id_, cmd_id_sz,10), task_id,
    CwxCommon::snprintf(subtask_id, subtask_id_sz, 10),svr_pool_str.c_str(),
    svr_pool_id, service_str.c_str(), ip_str.c_str(), cmt_type, state,
    err_msg_str.c_str(), uid);
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg_.c_str()));
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  return true;
}

inline bool DcmdCenterTaskMgr::ExecSql(DcmdTss* tss, bool is_commit) {
  if (-1 == mysql_->execute(tss->sql_)) {
    tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg_.c_str()));
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR((tss->err_msg.c_str()));
    mysql_->rollback();
    return false;
  }
  return true;
}

inline void DcmdCenterTaskMgr::RemoveTaskFromMem(DcmdCenterTask* task) {
  all_tasks_.erase(task->task_id_);
  map<uint64_t, DcmdCenterSubtask*>* subtasks;
  map<uint64_t, DcmdCenterSubtask*>::iterator subtasks_iter;
  map<string, DcmdCenterSvrPool*>::iterator pool_iter = task->pools_.begin();
  while(pool_iter != task->pools_.end()){
    subtasks = (map<uint64_t, DcmdCenterSubtask*>*)pool_iter->second->all_subtasks();
    subtasks_iter = subtasks->begin();
    while(subtasks_iter != subtasks->end()){
      if (subtasks_iter->second->exec_cmd_){
        RemoveCmd(subtasks_iter->second->exec_cmd_);
      }
      all_subtasks_.erase(subtasks_iter->second->subtask_id_);
      delete subtasks_iter->second;
      subtasks_iter++;
    }
    pool_iter++;
  }
  delete task;
}

inline void DcmdCenterTaskMgr::RemoveCmd(DcmdCenterCmd* cmd) {
  waiting_cmds_.erase(cmd->cmd_id_);
  if (cmd->agent_) {
    cmd->agent_->cmds_.erase(cmd->cmd_id_);
    if (!cmd->agent_->cmds_.size()){
      agents_.erase(cmd->agent_->ip_);
      delete cmd->agent_;
    }
  }
  if (cmd->subtask_) {
    if (cmd->subtask_->exec_cmd_ == cmd) {
      cmd->subtask_->exec_cmd_ = NULL;
    }
  }
  delete cmd;
}

}  // dcmd
