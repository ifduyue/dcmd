﻿#include "dcmd_def.h"

#include <CwxMd5.h>
#include <CwxFile.h>
namespace dcmd {
DcmdCenter::DcmdCenter(){
  host_id_ = 0;
  host_port_ = 0;
  conn_id_ = 0;
  last_heatbeat_time_ = 0;
  heatbeat_internal_ = kDefHeatbeatSecond;
  max_package_size_ = kDefMaxPackageMSize;
  is_connected_ = false;
  is_auth_ = false;
  is_auth_failed_ = false;
}

bool dcmd_parse_host_port(string const& host_port, CwxHostInfo& host) {
  if ((host_port.find(':') == string::npos) || (0 == host_port.find(':')))
    return false;
  host.setHostName(host_port.substr(0, host_port.find(':')));
  host.setPort(atoi(host_port.substr(host_port.find(':') + 1).c_str()));
  return true;
}

void dcmd_escape_mysql_string( string& value){
    CwxCommon::replaceAll(value, "\\", "\\\\");
    CwxCommon::replaceAll(value, "'", "\\'");
    CwxCommon::replaceAll(value, "\"", "\\\"");
}

void dcmd_md5(char const* content, uint32_t len, string& md5) {
  CwxMd5 md5_obj;
  unsigned char md5_sign[16];
  char md5_str[33];
  md5_obj.update((unsigned char const*)content, len);
  md5_obj.final(md5_sign);
  for (uint32_t i=0; i<16; i++){
    sprintf(md5_str + i *2, "%2.2x", md5_sign[i]);
  }
  md5 = md5_str;
}

void dcmd_remove_spam(string& value) {
  CwxCommon::replaceAll(name, "\n", "");
  CwxCommon::replaceAll(name, "\r", "");
  CwxCommon::replaceAll(name, " ", "");
  CwxCommon::replaceAll(name, "|", "");
  CwxCommon::replaceAll(name, "\"", "");
  CwxCommon::replaceAll(name, "'", "");
  CwxCommon::replaceAll(name, ";", "");
}
}