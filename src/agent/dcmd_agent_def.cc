#include "dcmd_agent_def.h"

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
}
