﻿#ifndef __DCMD_AGENT_DEF_H__
#define __DCMD_AGENT_DEF_H__

#include <CwxCommon.h>
#include <CwxHostInfo.h>

#include "../cmn/dcmd_def.h"
#include "../cmn/dcmd_tss.h"
#include "../cmn/dcmd.pb.h"
#include "dcmd_macro.h"
#include "dcmd_process.h"

namespace dcmd {
  ///控制中心对象
class DcmdCenter{
 public:
  DcmdCenter();

 public:
  // center的id
  uint32_t        host_id_;
  // center的名字
  string          host_name_;
  // center连接的端口
  uint16_t        host_port_;
  // center连接的连接id
  uint32_t        conn_id_;
  // center的上次心跳时间
  uint32_t        last_heatbeat_time_;
  // center的心跳间隔
  uint32_t        heatbeat_internal_;
  // center的通信数据包大小
  uint32_t        max_package_size_;
  // 是否与center建立了连接
  bool				    is_connected_;
  // 是否已经向center鉴权
  bool            is_auth_;
  // 是否鉴权失败
  bool            is_auth_failed_;
  // 错误信息
  string          err_msg_;
};

class DcmdAgentAppObj{
 public:
  DcmdAgentAppObj(){
    processor_ = NULL;
    running_cmd_ = NULL;
  }
  ~DcmdAgentAppObj() {
    if (processor_) delete processor_;
  }
 public:
  // 应用的名字
  string			                      app_name_;
  // 应用的命令
  list<dcmd_api::AgentTaskCmd*>	    cmds_;
  // 应用当前执行的进程
  DcmdProcess*                      processor_;
  // 当前正在运行的任务名字
  dcmd_api::AgentTaskCmd*           running_cmd_;
};

}  // dcmd

#endif