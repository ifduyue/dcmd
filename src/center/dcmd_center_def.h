﻿#ifndef __DCMD_CENTER_DEF_H__
#define __DCMD_CENTER_DEF_H__

#include <CwxCommon.h>
#include <CwxHostInfo.h>

#include "../cmn/dcmd_def.h"
#include "../cmn/dcmd_tss.h"
#include "../cmn/dcmd.pb.h"
#include "../cmn/dcmd_macro.h"

namespace dcmd {

class DcmdCenterTask;
class DcmdCenterSubTask;
class DcmdCenterAgent;

// 命令对象，为数据库command表中的记录
class DcmdCenterCmd{
 public:
 // 命令状态定义
 enum {
   // 命令未处理
   DCMD_CMD_STATE_UNDO = 0,
   // 命令已经完成
   DCMD_CMD_STATE_FINISH = 1,
   // 命令处理失败
   DCMD_CMD_STATE_FAIL = 2
  };
 public:
  DcmdCenterCmd() {
    cmd_id_ = 0;
    task_id_ = 0;
    subtask_id_ = 0;
    cmd_type_ = dcmd_api::CMD_UNKNOWN;
    state_ = DCMD_CMD_STATE_UNDO;
    task_ = NULL;
    subtask_ = NULL;
    agent_ = NULL;
  }
  ~DcmdCenterCmd() {
   }
 public:
  // cmd的id
  uint64_t                cmd_id_;
  // cmd对应的task id
  uint32_t                task_id_;
  // cmd对应的subtask id
  uint64_t                subtask_id_;
  // cmd对应的svr池子
  string                  svr_pool_;
  // cmd对应的service
  string                  service_;
  // cmd对应的agent的ip地址
  string                  agent_ip_;
  // 命令类型
  dcmd_api::CmdType       cmd_type_;
  // 命令的状态
  uint8_t                 state_;
  // 命令对应的任务对象
  DcmdCenterTask*         task_;
  // 命令对应的subtask对象
  DcmdCenterSubTask*      subtask_;
  // 命令对应的agent对象
  DcmdCenterAgent*        agent_;
};

// node对象的subtask定义，对应于task_node 表
class DcmdCenterSubtask{
 public:
  DcmdCenterSubtask() {
    subtask_id_ = 0;
    task_id_ = 0;
    state_ = dcmd_api::SUBTASK_INIT;
    is_ignored_ = false;
    start_time_ = 0;
    finish_time_ = 0;
    agent_ = NULL;
    task_ = NULL;
  }
public:
  // 子任务的id
  uint64_t                 subtask_id_;
  // 任务的id
  uint32_t                 task_id_;
  // 任务命令的名字
  string                   task_cmd_;
  // 子任务的服务池子名字
  string                   svr_pool_;
  // service的名字
  string                   service_;
  // subtask的ip
  string                   ip_;
  // 子任务的状态
  dcmd_api::SubTaskState   state_;
  // 是否被ignore
  bool                     is_ignored_;
  // 子任务的开始时间
  uint32_t                 start_time_;
  // 子任务的完成时间
  uint32_t                 finish_time_;
  // 子任务的进度
  string                   process_;
  // 子任务对应的agent对象
  DcmdCenterAgent*         agent_;
  // 子任务对应的task对象
  DcmdCenterTask*          task_;
};

// Task svr pool 对象，对应于task_service_pool表
class DcmdCenterSvrPool{
 public:
   DcmdCenterSvrPool(uint32_t task_id): task_id_(task_id) {
   }
   ~DcmdCenterSvrPool(){
   }

 public:
  // 往池子中添加subtask；true：成功；false表示存在
  bool AddSubtask(DcmdCenterSubtask* subtask);
  // 改变Subtask的状态；true：成功；false表示不存在
  bool ChangeSubtaskState(uint64_t subtask_id,
    dcmd_api::SubTaskState state,
    bool is_ignored);
public:
  // 是否可以调度
  inline bool EnableSchedule(uint32_t doing_num, // 当前正在做的数量
    uint32_t cont_num, // 最大并发的数量
    uint32_t doing_rate // 做的最大比率
    ) const
  {
    if (doing_num >= cont_num) return false;
    // doing_num加1的原因是计算若增加一台是否超过规定
    if ((doing_num + 1) * 100 > all_subtasks_.size() * doing_rate) return false;
    return true;
  }
  // 获取池子node的数量
  uint32_t total_host_num() const {
    return all_subtasks_.size();
  }
  // 获取未作node的数量
  uint32_t undo_host_num() const {
    return undo_subtasks_.size(); 
  }
  // 获取正在做的node的数量
  uint32_t doing_host_num() const {
    return doing_subtasks_.size();
  }
  // 获取ignore的正在做的node的数量
  uint32_t ignored_doing_host_num() const {
    return ignored_doing_subtasks_.size();
  }
  //获取已经完成的node的数量
  uint32_t finished_host_num() const {
    return finished_subtasks_.size();
  }
  // 获取失败的node的数量 
  uint32_t failed_host_num() const {
    return failed_subtasks_.size();
  }
  // 获取ignore的失败node的数量
  uint32_t ignored_failed_host_num() const {
    return ignored_failed_subtasks_.size();
  }
  // 获取所有的subtask
  map<uint64_t, DcmdCenterSubtask*> const* all_subtasks() const {
    return &all_subtasks_;
  }
  // 获取未作的subtask
  map<uint64_t, DcmdCenterSubtask*> const* undo_subtasks() const {
    return &undo_subtasks_;
  }
  //获取正在做的subtask
  map<uint64_t, DcmdCenterSubtask*> const* doing_subtasks() const {
    return &doing_subtasks_;
  }
  //获取被ignored的正在做的subtask
  map<uint64_t, DcmdCenterSubtask*> const* ignored_doing_subtasks() const {
    return &ignored_doing_subtasks_;
  }
  // 获取已经完成的subtask
  map<uint64_t, DcmdCenterSubtask*> const* finished_subtasks() const {
    return &finished_subtasks_;
  }
  // 获取失败的subtask
  map<uint64_t, DcmdCenterSubtask*> const* failed_subtasks() const {
    return &failed_subtasks_;
  }
  // 获取被ignored的失败subtask
  map<uint64_t, DcmdCenterSubtask*> const* ignored_failed_subtasks() const {
    return &ignored_failed_subtasks_;
  }

 public:
  // 任务的id
  uint32_t                          task_id_;
  // 任务的命令
  string                            task_cmd_;
  // 服务池子的名字
  string                            svr_pool_;
  // 服务池子的环境版本
  string                            svr_env_ver_;
  // 服务池子的版本库地址
  string                            repo_;
  // 服务池子的运行用户
  string                            run_user_;
 
 private:
  // 所有设备的subtask的map
  map<uint64_t, DcmdCenterSubtask*>      all_subtasks_;
  map<uint64_t, DcmdCenterSubtask*>      undo_subtasks_;
  map<uint64_t, DcmdCenterSubtask*>      doing_subtasks_;
  map<uint64_t, DcmdCenterSubtask*>      finished_subtasks_;
  map<uint64_t, DcmdCenterSubtask*>      failed_subtasks_;
  map<uint64_t, DcmdCenterSubtask*>      ignored_failed_subtasks_;
  map<uint64_t, DcmdCenterSubtask*>      ignored_doing_subtasks_;
};

// 任务对象，对应数据库的task表
class DcmdCenterTask {
 public:
  DcmdCenterTask() {
    task_id_ = 0;
    parent_task_id_ = 0;
    parent_task_ = NULL;
    doing_order_ = 0;
    order_ = 0;
    svr_id_ = 0;
    group_id_ = 0;
    update_env_ = false;
    update_tag_ = false;
    state_ = dcmd_api::TASK_INIT;
    max_current_num_ = 0;
    max_current_rate_ = 0;
    timeout_ = 0;
    is_auto_ = false;
    is_output_process_ = false;
  }
  ~DcmdCenterTask(){
    // 需要释放池子对象
    map<string, DcmdCenterSvrPool*>::iterator iter= pools_.begin();
    while(iter != pools_.end()){
      delete iter->second;
      iter++;
    }
  }

 public:
  // 往task中添加service池子；true 成功；false：存在
  bool AddSvrPool(DcmdCenterSvrPool* pool);
  // 获取task的池子
  DcmdCenterSvrPool* GetSvrPool(string const& pool_name);
  // 添加新subtask，true 成功；false：失败。失败或者subtask存在，或者pool不存在
  bool AddSubtask(DcmdCenterSubtask* subtask);
  // 改变任务的状态，true 成功；false：失败。失败或者subtask不存在，或者pool不存在
  bool ChangeSubtaskState(DcmdCenterSubtask const* subtask,
    dcmd_api::SubTaskState state,
    bool is_ignored)
 public:
  // 任务的id
  uint32_t                    task_id_;
  // 任务的名字
  string                      task_name_;
  // 任务的命令
  string                      task_cmd_;
  // 父任务的id
  uint32_t                    parent_task_id_;
  // 父任务
  DcmdCenterTask*             parent_task_;
  // 子任务的列表，按照顺序排序
  vector<list<DcmdCenterTask*> >  child_tasks_;
  // 当前任务执行到的index
  uint32_t                    doing_order_;
  // 执行顺序
  uint32_t                    order_;
  // service的id
  uint32_t                    svr_id_;
  // service的名字
  string                      service_;
  // group的id
  uint32_t                    group_id_;
  // group的名字
  string                      group_name_;
  // tag
  string                      tag_;
  // 是否更新环境
  bool                        update_env_;
  // 是否重新取版本
  bool                        update_tag_;
  // 任务的状态
  dcmd_api::TaskState         state_;
  // 并行执行的最大数量
  uint32_t                    max_current_num_;
  // 并行执行的最大比率
  uint32_t                    max_current_rate_;
  // 执行的超时时间
  uint32_t                    timeout_;
  // 是否自动执行
  bool                        is_auto_;
  // 是否输出进度信息
  bool                        is_output_process_;
  // 任务的参数
  list<string, string>        args_;
  // 任务的脚本
  string                      task_cmd_script_;
  // 任务脚本的md5值
  string                      task_cmd_script_md5_;
  // 任务的池子
  map<string, DcmdCenterSvrPool*>   pools_;
};

// agent节点命令对象，只有存在task命令的agent节点才存在
class DcmdCenterAgent{
 public:
  DcmdCenterAgent(string const& ip):ip_(ip) {
  }
  ~DcmdCenterAgent(){
    cmds_.clear();
  }
 public:
  // 设备的ip地址
  string      ip_;  ///设备的ip地址
  // 节点上待发送的命令, key为cmd的id
  map<uint64_t, DcmdCenterAgent*>  cmds_;
};

}  // dcmd

#endif
