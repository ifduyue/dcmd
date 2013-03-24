#include "dcmd_agent_def.h"

namespace dcmd {
AgentCenter::AgentCenter(){
  host_id_ = 0;
  host_port_ = 0;
  conn_id_ = 0;
  last_heatbeat_time_ = 0;
  opr_queue_threshold_ = kDefOprQueueThreshold;
  opr_overflow_threshold_ = kDefOprOverflowThreshold;
  heatbeat_internal_ = kDefHeatbeatSecond;
  max_package_size_ = kDefMaxPackageMSize;
  is_connected_ = false;
  is_auth_ = false;
  is_auth_failed_ = false;
}
}
