#ifndef __DCMD_DEF_H__
#define __DCMD_DEF_H__

#include <CwxCommon.h>
#include <CwxHostInfo.h>

#include "dcmd_macro.h"
#include "dcmd_cmd.pb.h"
#include "dcmd_agent.pb.h"
#include "dcmd_ui.pb.h"

namespace dcmd {

// 解析host:port的格式
bool dcmd_parse_host_port(string const& host_port, CwxHostInfo& host);
// escape mysql的特殊字符
void dcmd_escape_mysql_string(string& value);
// 获取内容的md5
void dcmd_md5(char const* content, uint32_t len, string& md5);
// 消除span的字符
void dcmd_remove_spam(string& value);

}  // dcmd

#endif
