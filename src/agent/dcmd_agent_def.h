﻿#ifndef __DCMD_AGENT_DEF_H__
#define __DCMD_AGENT_DEF_H__

#include <CwxCommon.h>
#include <CwxHostInfo.h>

#include "../cmn/dcmd_def.h"
#include "../cmn/dcmd_tss.h"
#include "../cmn/dcmd_macro.h"
#include "dcmd_process.h"

namespace dcmd {
  // 控制中心对象
class AgentCenter{
 public:
  AgentCenter();
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
  // queue的opr的threshold
  uint32_t        opr_queue_threshold_;
  // overflow的opr的数量
  uint32_t        opr_overflow_threshold_;
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

class AgentTaskResult {
 public:
  AgentTaskResult() {
    msg_taskid_ = 0;
    cmd_id_ = 0;
  }
 public:
  dcmd_api::AgentTaskResult    result_;
  uint32_t                     msg_taskid_;
  uint64_t                     cmd_id_;
};

class AgentTaskCmd {
 public:
  AgentTaskCmd() {
    msg_taskid_ = 0;
    cmd_id_ = 0;
    last_check_process_time_ = 0;
  }
 public:
  // 通信接口的对象
  dcmd_api::AgentTaskCmd       cmd_;
  // 消息的task id
  uint32_t                     msg_taskid_;
  // subtask的命令id
  uint64_t                     cmd_id_;
  // 上一次检查进度的时间点
  uint32_t                     last_check_process_time_;
  // 上一次检查的process
  string                       last_check_process_;
};

class AgentOprCmd {
 public:
   AgentOprCmd() {
     msg_taskid_ = 0;
     processor_ = NULL;
     center_ = NULL;
     opr_id_ = 0;
     agent_opr_id_ = 0;
     begin_time_ = 0;
   }
   ~AgentOprCmd() {
     if (processor_) delete processor_;
   }
 public:
   // 通信接口的对象
   dcmd_api::AgentOprCmd        cmd_;
   // 通信的task id
   uint32_t                     msg_taskid_;
   // 操作当前执行的进程
   DcmdProcess*                 processor_;
   // 发送操作的控制中心对象
   AgentCenter*                 center_;
   // 操作的id
   uint64_t                     opr_id_;
   // agent自增操作id
   uint64_t                     agent_opr_id_;
   // 运行时间
   uint32_t                     begin_time_;
};

class AgentSvrObj{
 public:
  AgentSvrObj(){
    processor_ = NULL;
    running_cmd_ = NULL;
  }
  ~AgentSvrObj() {
    if (processor_) delete processor_;
  }
 public:
  // service的名字
  string			                      svr_name_;
  // service的命令
  list<AgentTaskCmd*>	              cmds_;
  // service当前执行的进程
  DcmdProcess*                      processor_;
  // 当前正在运行的任务名字
  AgentTaskCmd*                     running_cmd_;
};

}  // dcmd

#endif
