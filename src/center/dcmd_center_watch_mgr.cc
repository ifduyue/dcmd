#include "dcmd_center_watch_mgr.h"
#include "dcmd_center_app.h"
namespace dcmd {
uint32_t DcmdCenterWatchMgr::AddWatch(uint32_t ui_client_id,
    uint32_t conn_id,
    uint32_t watch_task_id,
    bool is_watch_last_state)
{
  DcmdWatchObj* obj = new DcmdWatchObj();
  obj->conn_id_ = conn_id;
  obj->ui_client_id_ = ui_client_id;
  obj->is_watch_last_state_ = is_watch_last_state;
  obj->watched_task_id_ = watch_task_id;
  map<uint32_t, DcmdWatchSet* >::iterator iter = task_index_.find(watch_task_id);
  DcmdWatchSet* watch_set = NULL;
  if (iter_task == task_index_.end()) {
    watch_set = new DcmdWatchSet;
    task_index_[watch_task_id] = watch_set;
  } else {
    watch_set = iter->second;
    if (watch_set->find(obj) != watch_set->end()) {
      // watch 存在，只更新watch的状态并返回
      delete obj;
      (*watch_set->find(obj))->is_watch_last_state_ = is_watch_last_state;
      return watch_set->find(obj)->watch_id_;
    }
  }
  // watch 不存在
  obj->watch_id_ = watch_id();
  watch_set->insert(obj);
  // 将watch添加到conn_index_
  iter = conn_index_.find(conn_id);
  if (iter == conn_index_.end()) {
    watch_set = new DcmdWatchSet;
    conn_index_[conn_id] = watch_set;
  } else {
    watch_set = iter->second;
  }
  watch_set->insert(obj);
  // 添加到watches_
  watches_[obj->watch_id_] = obj;
  return obj->watch_id_;
}

uint32_t DcmdCenterWatchMgr::GetWatchId(uint32_t ui_client_id,
  uint32_t conn_id,
  uint32_t watch_task_id) 
{
  DcmdWatchObj obj;
  obj.conn_id_ = conn_id;
  obj.ui_client_id_ = ui_client_id;
  obj.watched_task_id_ = watch_task_id;
  map<uint32_t, DcmdWatchSet* >::const_iterator iter = task_index_.find(watch_task_id);
  if (iter == task_index_.end()) return 0;
  DcmdWatchSet_Iter watch_iter = (*iter->second)->find(&obj);
  if (watch_iter == (*iter->second)->end()) return 0;
  return (*watch_iter)->watch_id_;
}


bool DcmdCenterWatchMgr::ExistWatch(uint32_t ui_client_id,
    uint32_t conn_id,
    uint32_t watch_task_id) const
{
  return GetWatchId(ui_client_id, conn_id, watch_task_id) != 0;
}

void DcmdCenterWatchMgr::CancelWatch(uint32_t ui_client_id,
    uint32_t conn_id,
    uint32_t watch_task_id)
{
  uint32_t watch_id = GetWatchId(ui_client_id, conn_id, watch_task_id);
  if (!watch_id) return;
  DcmdWatchObj* obj = watches_.find(watch_id)->second;
  map<uint32_t, DcmdWatchSet* >::const_iterator iter = task_index_.find(watch_task_id);
  CWX_ASSERT(iter != task_index_.end());
  DcmdWatchSet* watch_set = iter->second;
  watch_set->erase(obj);
  if (!watch_set->size()) {
    task_index_.erase(iter);
    delete watch_set;
  }
  iter = conn_index_.find(conn_id);
  CWX_ASSERT(iter != conn_index_.end());
  watch_set = iter->second;
  watch_set->erase(obj);
  if (!watch_set->size()) {
    conn_index_.erase(iter);
    delete watch_set;
  }
  watches_.erase(watch_id);
  delete obj;
}

void DcmdCenterWatchMgr::CancelWatch(uint32_t conn_id) {

}

void DcmdCenterWatchMgr:GetWatchesByConn(list<DcmdWatchObj>& watches) {

}

void DcmdCenterWatchMgr::GetWatches(list<DcmdWatchObj>& watches) {

}

void DcmdCenterWatchMgr::NoticeTaskChange(DcmdCenterTask const& task) {

}

void DcmdCenterWatchMgr::Clear(bool is_close_conn) {

}

}  // dcmd

