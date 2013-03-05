#ifndef __DCMD_DEF_H__
#define __DCMD_DEF_H__

#include <CwxCommon.h>
#include <CwxHostInfo.h>

#include "dcmd_macro.h"
#include "dcmd.pb.h"

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

///解析host:port的格式
bool dcmd_parse_host_port(string const& host_port, CwxHostInfo& host);
///escape mysql的特殊字符
void dcmd_escape_mysql_string(string& value);
///获取内容的md5
void dcmd_md5(char const* content, uint32_t len, string& md5);
///消除span的字符
void dcmd_remove_spam(string& value);

}  // dcmd

#endif
