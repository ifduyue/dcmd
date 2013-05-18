namespace dcmd {

inline DcmdCenterTask* DcmdCenterTaskMgr::GetTask(uint32_t task_id) {
  map<uint32_t, DcmdCenterTask*>::iterator iter = all_tasks_.find(task_id);
  if (iter == all_tasks_.end()) return NULL;
  return iter->second;
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
    CWX_ERROR(tss->err_msg.c_str());
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR(tss->err_msg.c_str());
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
      CWX_ERROR((tss->err_msg_.c_str());
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + ". sql:" + tss->sql_;
      CWX_ERROR(tss->err_msg.c_str());
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
    CWX_ERROR((tss->err_msg_.c_str());
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR(tss->err_msg.c_str());
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
    CWX_ERROR((tss->err_msg_.c_str());
    mysql_->rollback();
    return false;
  }
  if (is_commit && !mysql_->commit()) {
    tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
      + ". sql:" + tss->sql_;
    CWX_ERROR(tss->err_msg.c_str());
    mysql_->rollback();
    return false;
  }
  return true;
}

 

}  // dcmd
