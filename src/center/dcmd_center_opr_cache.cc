#include "dcmd_center_opr_cache.h"

namespace dcmd {
void DcmdCenterOprCache::AddOprCmd(uint64_t opr_cmd_id, DcmdCenterOprCmd const& cmd) {
  CwxMutexGuard<CwxMutexLock>  lock(&lock_);
  map<uint64_t, DcmdCenterOprCmd*>::iterator iter = cmd_cache_.find(opr_cmd_id);
  if(iter != cmd_cache_.end()){
    *iter->second = cmd;
    iter->second->opr_cmd_id_ = opr_cmd_id;
  }else{
    DcmdCenterOprCmd* pcmd = new DcmdCenterOprCmd(cmd);
    pcmd->opr_cmd_id_ = opr_cmd_id;
    cmd_cache_[opr_cmd_id] = pcmd;
  }
}

bool DcmdCenterOprCache::GetOprCmd(uint64_t opr_cmd_id, DcmdCenterOprCmd& cmd) {
  CwxMutexGuard<CwxMutexLock>  lock(&lock_);
  uint32_t now = time(NULL);
  map<uint64_t, DcmdCenterOprCmd*>::iterator iter = cmd_cache_.find(opr_cmd_id);
  if(iter != cmd_cache_.end()){
    if (iter->second->expire_time_ < now) {
      cmd_cache_.erase(iter);
      return false;
    }
    cmd = *iter->second;
    return true;
  }
  return false;
}

void DcmdCenterOprCache::CheckTimeout(uint32_t now){
  CwxMutexGuard<CwxMutexLock>  lock(&lock_);
  uint64_t cur_opr_cmd_id = 0;
  map<uint64_t, DcmdCenterOprCmd*>::iterator iter = cmd_cache_.begin();
  while(iter != cmd_cache_.end()){
    if (iter->second->expire_time_ <= now){
      cur_opr_cmd_id = iter->first;
      cmd_cache_.erase(iter);
      iter = cmd_cache_.upper_bound(cur_opr_cmd_id);
    }else{
      ++iter;
    }
  }
}
}  // dcmd