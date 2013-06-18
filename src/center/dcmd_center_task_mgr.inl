namespace dcmd {

  inline DcmdCenterTask* DcmdCenterTaskMgr::GetTask(uint32_t task_id) {
    map<uint32_t, DcmdCenterTask*>::iterator iter = all_tasks_.find(task_id);
    if (iter == all_tasks_.end()) return NULL;
    return iter->second;
  }

  inline DcmdCenterSubtask* DcmdCenterTaskMgr::GetSubTask(uint64_t subtask_id) {
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

  inline bool DcmdCenterTaskMgr::UpdateTaskValid(DcmdTss* tss, bool is_commit, 
    uint32_t task_id, bool is_valid, char const* err_msg)
  {
    string str_tmp = err_msg?string(err_msg):"";
    dcmd_escape_mysql_string(str_tmp);
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
      "update task set valid=%d, errmsg='%s' where task_id=%d",
      is_valid?1:0, is_valid?"":str_tmp.c_str(), task_id);
    if (-1 == mysql_->execute(tss->sql_)) {
      tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    return true;
  }

  inline bool DcmdCenterTaskMgr::UpdateTaskState(DcmdTss* tss, bool is_commit,
    uint32_t task_id, uint8_t state) {
      CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize, 
        "update task set state=%d where task_id=%d", state);
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
      state, str_tmp.c_str(), CwxCommon::toString(subtask_id, buf, 10));
    if (-1 == mysql_->execute(tss->sql_)) {
      tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
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
      state, str_tmp.c_str(), CwxCommon::toString(cmd_id, buf, 10));
    if (-1 == mysql_->execute(tss->sql_)) {
      tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
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
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
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
        + string(". sql:") + tss->sql_;
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
      uid, task->task_id_, task->task_id_);
    if (-1 == mysql_->execute(tss->sql_)) {
      tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    return true;
  }

  inline uint64_t DcmdCenterTaskMgr::InsertCommand(DcmdTss* tss, bool is_commit, uint32_t uid,
    uint32_t task_id, uint64_t subtask_id, char const* svr_pool,
    uint32_t svr_pool_id, char const* service, char const* ip,
    uint8_t cmt_type, uint8_t state, char const* err_msg)
  {
    string svr_pool_str(svr_pool?svr_pool:"");
    string service_str(service?service:"");
    string ip_str(ip?ip:"");
    string err_msg_str(err_msg?err_msg:"");
    dcmd_escape_mysql_string(svr_pool_str);
    dcmd_escape_mysql_string(service_str);
    dcmd_escape_mysql_string(ip_str);
    dcmd_escape_mysql_string(err_msg_str);
    char cmd_id_sz[65];
    char subtask_id_sz[65];
    uint64_t cmd_id = ++next_cmd_id_;
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "insert into command(cmd_id, task_id, subtask_id, svr_pool, svr_pool_id, service, ip,"\
      "cmd_type, state, errmsg, utime, ctime, opr_uid) "\
      " values (%s, %u, %s, '%s', %u, '%s', %u, %u, '%s', now(), now(), %u)",
      CwxCommon::toString(cmd_id, cmd_id_sz,10), task_id,
      CwxCommon::toString(subtask_id, subtask_id_sz, 10),svr_pool_str.c_str(),
      svr_pool_id, service_str.c_str(), ip_str.c_str(), cmt_type, state,
      err_msg_str.c_str(), uid);
    if (-1 == mysql_->execute(tss->sql_)) {
      tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return 0;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return 0;
    }
    return cmd_id;
  }

  inline bool DcmdCenterTaskMgr::ExecSql(DcmdTss* tss, bool is_commit) {
    if (-1 == mysql_->execute(tss->sql_)) {
      tss->err_msg_ = string("Failure to exec sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()) {
      tss->err_msg_ = string("Failure to commit sql, err:") + mysql_->getErrMsg(),
        + string(". sql:") + tss->sql_;
      CWX_ERROR((tss->err_msg_.c_str()));
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
      subtasks = &pool_iter->second->all_subtasks_;
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

  inline bool DcmdCenterTaskMgr::UpdateSubtaskInfo(DcmdTss* tss, uint64_t subtask_id,
    bool is_commit, uint32_t* state, bool* is_skip,
    bool is_start_time, bool is_finish_time, char const* err_msg, 
    char const* process)
  {
    char tmp_buf[64];
    string value;
    string sql;
    uint32_t init_len = 0;
    CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
      "update task_node set ");
    sql = tss->sql_;
    init_len = sql.length();
    if (state){
      CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
        " state=%d ", *state);
      if (sql.length() != init_len) sql += ",";
      sql += tss->sql_;
    }
    if (is_skip){
      CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
        " skip=%d ", *is_skip?1:0);
      if (sql.length() != init_len) sql += ",";
      sql += tss->sql_;
    }
    if (is_start_time){
      CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
        " start_time=now() ");
      if (sql.length() != init_len) sql += ",";
      sql += tss->sql_;
    }
    if (is_finish_time){
      CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
        " finish_time=now() ");
      if (sql.length() != init_len) sql += ",";
      sql += tss->sql_;
    }
    if (err_msg){
      value = err_msg;
      dcmd_escape_mysql_string(value);
      CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
        " errmsg='%s' ", value.c_str());
      if (sql.length() != init_len) sql += ",";
      sql += tss->sql_;
    }
    if (process){
      value = process;
      dcmd_escape_mysql_string(value);
      CwxCommon::snprintf(tss->sql_, DcmdTss::kMaxSqlBufSize,
        " process='%s' ", value.c_str());
      if (sql.length() != init_len) sql += ",";
      sql += tss->sql_;
    }
    CwxCommon::snprintf(tss->sql_, DcmdTss::MAX_SQL_BUF_SIZE,
      ", update_time=now() where subtask_id = %s ", 
      CwxCommon::toString(subtask_id, szTmp, 10));
    sql += tss->sql_;

    if (-1 == mysql_->execute(sql.c_str())){
      CWX_ERROR(("Failure to exec sql, err:%s; sql:%s", mysql_->getErrMsg(), sql.c_str()));
      mysql_->rollback();
      return false;
    }
    if (is_commit && !mysql_->commit()){
      CWX_ERROR(("Failure to commit sql, err:%s; sql:%s", mysql_->getErrMsg(), sql.c_str()));
      mysql_->rollback();
      return false;
    }
    return true;
  }

  inline void DcmdCenterTaskMgr::FillCtrlCmd(dcmd_api::AgentTaskCmd& cmd,
    uint64_t cmd_id,
    dcmd_api::CmdType cmd_type,
    string const& agent_ip,
    string const& svr_name,
    DcmdCenterSubtask* subtask
    )
  {
    char buf[64];
    CwxCommon::toString(buf, cmd_id, 10);
    cmd.set_cmd(buf);
    cmd.set_task_cmd(kDcmdSysCmdCancel);
    cmd.set_cmd_type(cmd_type);
    if (subtask) {
      sprintf(buf, "%u", subtask->task_id_);
      cmd.set_task_id(buf);
      CwxCommon::toString(buf, subtask?subtask->subtask_id_:0, 10);
      cmd.set_subtask_id(buf);
    }
    cmd.set_ip(agent_ip);
    cmd.set_svr_name(svr_name);
    cmd.set_svr_pool("");
  }
  inline void DcmdCenterTaskMgr::FillTaskCmd(dcmd_api::AgentTaskCmd& cmd,
    uint64_t cmd_id,
    DcmdCenterSubtask const& subtask) 
  {
    char buf[64];
    CwxCommon::toString(buf, cmd_id, 10);
    cmd.set_cmd(buf);
    cmd.set_task_cmd(subtask.task_->task_cmd_);
    cmd.set_cmd_type(dcmd_api::CMD_DO_SUBTASK);
    sprintf(buf, "%u", subtask.task_->task_id_);
    cmd.set_task_id(buf);
    CwxCommon::toString(buf, subtask.subtask_id_, 10);
    cmd.set_subtask_id(buf);
    cmd.set_ip(subtask.ip_);
    cmd.set_svr_name(subtask.task_->service_);
    cmd.set_svr_pool(subtask.svr_pool_name_);
    cmd.set_svr_ver(subtask.task_->tag_);
    cmd.set_svr_repo(subtask.svr_pool_->repo_);
    cmd.set_svr_user(subtask.svr_pool_->run_user_);
    cmd.set_env_ver(subtask.svr_pool_->svr_env_ver_);
    cmd.set_update_env(subtask.task_->update_env_);
    cmd.set_update_ver(subtask.task_->update_tag_);
    cmd.set_output_process(subtask.task_->is_output_process_);
    cmd.set_script(subtask.task_->task_cmd_script_);
    dcmd_api::KeyValue* kv = NULL;
    map<string, string>::iterator iter = subtask.task_->args_.begin();
    while (iter != subtask.task_->args_.end()) {
      kv = cmd.task_arg().add_task_arg();
      kv.set_key(iter->first);
      kv.set_value(iter->second);
      ++iter;
    }
  }


}  // dcmd
