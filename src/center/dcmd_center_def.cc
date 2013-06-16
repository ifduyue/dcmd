#include "dcmd_center_def.h"

namespace dcmd {

bool DcmdCenterSvrPool::AddSubtask(DcmdCenterSubtask* subtask) {
  if (all_subtasks_.find(subtask->subtask_id_) != all_subtasks_.end())
    return false;
  all_subtasks_[subtask->subtask_id_] = subtask;
  switch(subtask->state_) {
  case dcmd_api::SUBTASK_DOING:
    if (subtask->is_ignored_) {
      ignored_doing_subtasks_[subtask->subtask_id_] = subtask;
    } else {
      doing_subtasks_[subtask->subtask_id_] = subtask;
    }
    break;
  case dcmd_api::SUBTASK_FINISHED:
    finished_subtasks_[subtask->subtask_id_] = subtask;
    break;
  case dcmd_api::SUBTASK_FAILED:
    if (subtask->is_ignored_) {
      ignored_failed_subtasks_[subtask->subtask_id_] = subtask;
    } else {
      failed_subtasks_[subtask->subtask_id_] = subtask;
    }
    break;
  case dcmd_api::SUBTASK_INIT:
    undo_subtasks_[subtask->subtask_id_] = subtask;
    break;
  default:
    CWX_ASSERT(0);
  }
  return true;
}

bool DcmdCenterSvrPool::ChangeSubtaskState(uint64_t subtask_id,
  dcmd_api::SubTaskState state,
  bool is_ignored)
{
  // 首先删除subtask
  map<uint64_t, DcmdCenterSubtask*>::iterator iter = all_subtasks_.find(subtask_id);
  if (iter == all_subtasks_.end()) return false;
  DcmdCenterSubtask* subtask = iter->second;
  if ((subtask->state_ == state) && (subtask->is_ignored_ == is_ignored)) return true;
  all_subtasks_.erase(iter);
  switch(subtask->state_) {
  case dcmd_api::SUBTASK_DOING:
    if (subtask->is_ignored_) {
      iter = ignored_doing_subtasks_.find(subtask_id);
      CWX_ASSERT(iter != ignored_doing_subtasks_.end());
      ignored_doing_subtasks_.erase(iter);
    } else {
      iter = doing_subtasks_.find(subtask_id);
      CWX_ASSERT(iter != doing_subtasks_.end());
      doing_subtasks_.erase(iter);
    }
    break;
  case dcmd_api::SUBTASK_FINISHED:
    iter = finished_subtasks_.find(subtask_id);
    CWX_ASSERT(iter != finished_subtasks_.end());
    finished_subtasks_.erase(iter);
    break;
  case dcmd_api::SUBTASK_FAILED:
    if (subtask->is_ignored_) {
      iter = ignored_failed_subtasks_.find(subtask_id);
      CWX_ASSERT(iter != ignored_failed_subtasks_.end());
      ignored_failed_subtasks_.erase(iter);
    } else {
      iter = failed_subtasks_.find(subtask_id);
      CWX_ASSERT(iter != failed_subtasks_.end());
      failed_subtasks_.erase(iter);
    }
    break;
  case dcmd_api::SUBTASK_INIT:
    iter = undo_subtasks_.find(subtask_id);
    CWX_ASSERT(iter != undo_subtasks_.end());
    undo_subtasks_.erase(iter);
    break;
  default:
    CWX_ASSERT(0);
  }
  subtask->is_ignored_ = is_ignored;
  subtask->state_ = state;
  return AddSubtask(subtask);
}

bool DcmdCenterTask::AddSvrPool(DcmdCenterSvrPool* pool) {
  if (pools_.find(pool->svr_pool_) != pools_.end()) return false;
  pools_[pool->svr_pool_] = pool;
  return true;
}

DcmdCenterSvrPool* DcmdCenterTask::GetSvrPool(string const& pool_name) {
  map<string, DcmdCenterSvrPool*>::iterator iter = pools_.find(pool_name);
  if ( iter == pools_.end()) return NULL;
  return iter->second;
}
// 获取池子的id
uint32_t DcmdCenterTask::GetSvrPoolId(string const& pool_name) {
  map<string, DcmdCenterSvrPool*>::iterator iter = pools_.find(pool_name);
  if ( iter == pools_.end()) return 0;
  return iter->second->svr_pool_id_;
}

bool DcmdCenterTask::AddSubtask(DcmdCenterSubtask* subtask) {
  map<string, DcmdCenterSvrPool*>::iterator iter = pools_.find(subtask->svr_pool_);
  if ( iter == pools_.end()) return false;
  subtask->task_ = this;
  return iter->second->AddSubtask(subtask);
}

bool DcmdCenterTask::ChangeSubtaskState(DcmdCenterSubtask const* subtask,
  dcmd_api::SubTaskState state,
  bool is_ignored)
{
  map<string, DcmdCenterSvrPool*>::iterator iter = pools_.find(subtask->svr_pool_);
  if ( iter == pools_.end()) return false;
  return iter->second->ChangeSubtaskState(subtask->subtask_id_, state, is_ignored);
}

void DcmdCenterTask::AddChildTask(DcmdCenterTask* child_task) {
  CWX_ASSERT(this->is_cluster_);
  CWX_ASSERT(task_id_ == child_task->parent_task_id_);
  map<uint32_t, map<uint32_t, DcmdCenterTask*>* >::iterator iter = child_tasks_.find(child_task->order_);
  map<uint32_t, DcmdCenterTask*>* tasks = NULL;
  if (iter == child_tasks_.end()) {
    tasks = new map<uint32_t, DcmdCenterTask*>;
  } else {
    tasks = iter->second;
  }
  child_task->parent_task_ = this;
  (*tasks)[child_task->task_id_] = child_task;
  if (child_task->state_ != dcmd_api::TASK_INIT) {
    if (this->doing_order_ < child_task->order_) {
      this->doing_order_ = child_task->order_;
    }
  }
}


}  // dcmd
