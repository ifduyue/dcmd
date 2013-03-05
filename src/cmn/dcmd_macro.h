#ifndef __DCMD_MACRO_H__
#define __DCMD_MACRO_H__

#include <CwxGlobalMacro.h>
#include <CwxStl.h>
#include <CwxStlFunc.h>
#include <CwxType.h>

CWINUX_USING_NAMESPACE

namespace dcmd {

// 操作指令的参数的前缀
const char* const kAgentOprArgPrex = "DCMD";

// 操作指令的缺省超时时间
const uint32_t kDefOprCmdTimeoutSecond = 30;
// 操作指令的最小超时时间
const uint32_t kMinOprCmdTimeoutSecond = 2;
// 操作指令最大的超时时间
const uint32_t kMaxOprCmdTimeoutSecond = 3600;  

// 缺省的设备ip表的刷新间隔
const uint32_t kDefIpTableRefreshSecond = 300;
// 最大的设备ip表的刷新间隔
const uint32_t kMaxIpTableRefreshSecond = 1800;
// 最小的设备ip表的刷新间隔
const uint32_t kMinIpTableRefreshSecond = 30;

// 缺省的请求超时时间
const uint32_t kDefReqTimeoutSecond = 10;
// 最小的请求超时时间
const uint32_t kMinReqTimeoutSecond = 1;
// 最大的请求超时时间
const uint32_t kMaxReqTimeoutSecond = 600;

// 缺省的数据包的最大大小
const uint32_t kDefMaxPackageMSize = 4;
// 最小的数据包的最大大小
const uint32_t kMinMaxPackageMSize = 2;
// 最大的数据包的最大大小
const uint32_t kMaxMaxPackageMSize = 16;

// agent ip的最大数量
const uint32_t kAgentMaxIpNum = 32;
// agent ip的最大长度,支持ip V6
const uint32_t kIpMaxLen =  40;

// 缺省的心跳时间
const uint32_t kDefHeatbeatSecond = 10;
// 最大的心跳时间
const uint32_t kMaxHeatbeatSecond = 600;
// 最小的心跳时间
const uint32_t kMinHeatbeatSecond = 1;

// 任务指令执行的stdout、stderr的输出日志保存时间
const uint32_t kTaskCmdOutputExpireDay = 7;

// 在Agent最多并行运行的操作指令脚本数量
const uint32_t kMaxAgentOprNum = 30;
// Center的master切换时间间隔
const uint32_t kCenterMasterSwitchTimeoutSecond = 120;
// Center的master检查时间间隔
const uint32_t kCenterMasterCheckSecond = 5;

// 缺省的日志文件数量
const uint32_t kDefLogFileNum = 7;
// 最小的日志文件数量
const uint32_t kMinLogFileNum = 3;
// 最大的日志文件数量
const uint32_t kMaxLogFileNum = 100;

// 缺省的日志文件大小
const uint32_t kDefLogFileMSize = 10;
// 最小的日志文件大小
const uint32_t kMinLogFileMSize = 1;
// 最大的日志文件大小
const uint32_t kMaxLogFileMSize = 100;

// 字符型的【;】分割符
const char  kItemSplitChar = ';';
// 字符串型的【;】分隔符
const char* const kItemSplitStr = ";";
// 字符型的换行符
const char  kLineSplitChar = '\n';
// 字符串型的换行符
const char* const kLineSplitStr = "\n";

// 任务的执行结果输出目录
const char* const kAgentTaskCmdOutputSubPath = "task_output";
// 操作的执行结果输出目录
const char* const kAgentOprCmdOutputSubPath = "opr_output";
// 任务的script存储目录
const char* const kCenterTaskScriptSubPath = "task_script";
// 操作的script存储目录
const char* const kCenterOprScriptSubPath = "opr_script";
}  // dcmd
#endif
