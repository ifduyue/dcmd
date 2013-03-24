#ifndef __DCMD_CENTER_CONFIG_H__
#define __DCMD_CENTER_CONFIG_H__

#include <CwxCommon.h>
#include <CwxIniParse.h>
#include <CwxHostInfo.h>

#include "dcmd_center_def.h"

namespace dcmd {

// 配置文件的common参数对象
class DcmdCenterConfCmn{
 public:
  DcmdCenterConfCmn(){
    heatbeat_internal_ = kDefHeatbeatSecond;
    agent_package_size_ = kDefMaxPackageMSize;
    ip_refresh_interanl_ = kDefIpTableRefreshSecond;
    illegal_agent_block_second_ = kDefIlegalAgentBlockSecond; 
    is_allow_opr_cmd_ = true;
    log_file_num_ = kDefLogFileNum;
    log_file_size_ = kDefLogFileMSize;
    is_debug_ = false;
    is_opr_cmd_arg_mutable_ = false;
    is_opr_cmd_history_ = true;
  }
 public:
  // 工作目录
  string              work_home_;
  // center的主机标识
  string              host_id_;
  // agent监听地址
  CwxHostInfo         agent_listen_;
  // UI的监听地址
  CwxHostInfo         ui_listen_;
  // 心跳的时间间隔
  uint32_t            heatbeat_internal_;
  // agent通信的package大小
  uint32_t            agent_package_size_;
  // node ip的刷新间隔
  uint32_t            ip_refresh_interanl_;
  // 操作的用户名
  string               ui_user_name_;
  // 操作的用户口令
  string               ui_user_passwd_;
  // 操作指令的script的安装路径
  string               opr_script_path_;
  // 任务脚本的安装路径
  string               task_script_path_;
  // 非法agent的block时间
  uint32_t             illegal_agent_block_second_; 
  // 是否运行执行操作指令
  bool                 is_allow_opr_cmd_; 
  // 日志文件的数量
  uint32_t             log_file_num_;
  // 日志文件的大小
  uint32_t             log_file_size_;
  // 是否打开调试开关
  bool                 is_debug_;
  // opr cmd的命令参数是否可变
  bool                 is_opr_cmd_arg_mutable_;
  // opr cmd执行是否必须记录history
  bool                 is_opr_cmd_history_;
  // 运行连接center的ip地址，可以为c类段
  set<string>          allow_ui_ips_;
};

// 控制中心的mysql连接信息
class DcmdCenterConfMysql{
 public:
  DcmdCenterConfMysql(){
    port_ = 3306;
  }
 public:
  // mysql的server
  string			   server_;
  // mysql的port
  uint16_t       port_;
  // mysql的用户
  string			   user_;
  // mysql的用户
  string			   passwd_;
  // mysql的数据库
  string			   db_name_;
};

// 配置文件加载对象
class DcmdCenterConf{
 public:
  DcmdCenterConf(){
    err_msg_[0] = 0x00;
  }
  ~DcmdCenterConf(){
  }
 public:
  // 加载配置文件.-1:failure, 0:success
  int Init(string const& conf_file);
  // 输出加载的配置文件信息
  void OutputConfig() const;

 public:
  // 获取common配置信息
  inline DcmdCenterConfCmn const& common() const {
    return  common_;
  }
  // 获取mysql的配置信息
  inline DcmdCenterConfMysql const& mysql() const{
    return mysql_;
  }
  // 获取配置文件加载的失败原因
  inline char const* err_msg() const{
    return err_msg_;
  };
  // 获取任务指令的文件名
  inline static string& task_cmd_file(string const& task_cmd, string& cmd_file) {
    cmd_file = string(kTaskTypeFilePrex) + task_cmd + kTaskTypeFileSuffix;
    return cmd_file;
  }
  // 获取操作指令的文件名
  inline static string& opr_cmd_file(string const& opr_name, string& cmd_file) {
    cmd_file = string(kOprCmdFilePrex) + opr_name + kOprCmdFileSuffix;
    return cmd_file;
  }
 private:
  // common配置信息
  DcmdCenterConfCmn       common_;
  //mysql的配置
  DcmdCenterConfMysql     mysql_;
  // 错误消息的buf
  char                    err_msg_[2048];
};

}  // dcmd

#endif