#ifndef __DCMD_CENTER_TASK_MGR__
#define __DCMD_CENTER_TASK_MGR__

#include "dcmd_center_def.h"
#include "dcmd_center_h4_agent_task.h"
#include "dcmd_mysql.h"
#include "dcmd_xml_parse.h"
#include "dcmd_center_task_mgr.h"

namespace dcmd {
class DcmdCenterApp;

// 任务管理类，此对象多线程不安全
// 除了GetAgentTaskProcess外，其他api全部由任务线程调用。
// 只有GetAgentTaskProcess/SetAgentTaskProcess/DelAgentsTaskProcess三个api是多线程安全的.
class DcmdCenterTaskMgr{
 public:
  DcmdCenterTaskMgr(DcmdCenterApp* app);
  ~DcmdCenterTaskMgr();
 public:    
  // 启动命令处理，若返回false是数据库操作失败
  bool Start(DcmdTss* tss);
  // 停止命令处理，若返回false是数据库操作失败
  void Stop(DcmdTss* tss);
  // 接收新指令
  bool ReceiveCmd(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd,
    uint32_t  conn_id,
    uint32_t  msg_taskid);
  // agnet的master通知回复通知处理函数，若返回false是数据库操作失败
  bool ReceiveAgentMasterReply(DcmdTss* tss, ///<tss对象
    string const& agent_ip,
    list<string> const& agent_cmds
    );
  // agent连接关闭通知，若返回false是数据库操作失败
  bool ReceiveAgentClosed(DcmdTss* tss, string const& agent_ip);
  // ui的连接关闭通知，此会关闭watch
  bool ReceiveUiClosed(DcmdTss* tss, CWX_UINT32 conn_id);
  // 收到agent的命令确认消息处理，若返回false是数据库操作失败
  bool ReceiveAgentSubtaskConfirm(DcmdTss* tss,
    string const& agent_ip,
    string cmd_id);
  // 收到agent的命令处理结果，若返回false是数据库操作失败
  bool ReceiveAgentSubtaskResult(DcmdTss* tss,
    uint32_t msg_taskid,
    dcmd_api::AgentTaskResult const& result
    );
  // 设置agent上的任务处理进度
  void SetAgentTaskProcess(string const& task_id,
    string const& subtask_id,
    char const* process);
  // 获取agent上的任务处理进度, false表示不存在
  bool GetAgentsTaskProcess(string const& subtask_id, string& process);

 private:
  // 清空对象
  void Reset();
  // 启动任务
  dcmd_api::DcmdState TaskCmdStartTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 暂停任务
  dcmd_api::DcmdState TaskCmdPauseTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 继续任务
  dcmd_api::DcmdState TaskCmdResumeTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 完成任务
  dcmd_api::DcmdState TaskCmdFinishTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // cancel具体subtask的执行
  dcmd_api::DcmdState TaskCmdCancelSubtask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // cancel一个服务的所有任务的执行
  dcmd_api::DcmdState TaskCmdCancelSvrSubtask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 重做整个任务
  dcmd_api::DcmdState TaskCmdRedoTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 重做一个服务池子
  dcmd_api::DcmdState TaskCmdRedoSvrPool(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 重做一个subtask
  dcmd_api::DcmdState TaskCmdRedoSubtask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 重做失败的subtask
  dcmd_api::DcmdState TaskCmdRedoFailedSubtask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 重做一个服务池子中的失败subtask
  dcmd_api::DcmdState TaskCmdRedoFailedSvrPoolSubtask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // ignore某个subtask的结果
  dcmd_api::DcmdState TaskCmdIgnoreSubtask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 冻结任务的执行
  dcmd_api::DcmdState TaskCmdFreezeTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 解除对一个任务的冻结
  dcmd_api::DcmdState TaskCmdUnfreezeTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 通知任务信息改变
  dcmd_api::DcmdState TaskCmdUpdateTask(DcmdTss* tss,
    dcmd_api::UiTaskCmd const& cmd);
  // 加载所有的数据
  bool LoadAllDataFromDb(DcmdTss* tss);
  // 从数据库中获取新task
  bool LoadNewTask(DcmdTss* tss);

  // 对所有的任务进行调度，若返回false，是数据库操作失败。
  bool Schedule(DcmdTss* tss);
  // 调度指定任务的指令，若返回false是数据库操作失败
  bool Schedule(DcmdTss* tss, 
        DcmdCenterTask* task);
  // 子任务完成处理操作，返回对应的task。若返回false是数据库操作失败
  bool FinishTaskCmd(DcmdTss* tss,
    dcmd_api::AgentTaskResult const& result,
    string& agent_ip,
    DcmdCenterTask*& task
    );
 private:
  // app对象
  DcmdCenterApp*                               app_;
  // 保护all_subtasks_的访问，并发访问process
  CwxMutexLock                                 lock_;
  
  /****** 一下的变量全部是多线程不安全的  *****/
  // 是否已经启动
  bool                                         is_start_;
  // mysql 的句柄
  Mysql*                                       mysql_;
  // 当前有待处理subtask的agent
  map<string, DcmdCenterAgent*>                agents_;
  // 当前所有的task
  map<uint32_t, DcmdCenterTask*>               all_tasks_;
  // 当前的所有subtask。subtask的状态、ignore必须通过svr-pool对象修改
  map<uint64_t, DcmdCenterSubtask*>            all_subtasks_;
  // 当前所有的等待回复的命令
  map<uint64_t, DcmdCenterCmd*>                waiting_cmds_;
  // 下一个的task的id
  uint32_t                                     next_task_id_;
  // 下一个subtask id
  uint64_t                                     next_subtask_id_;
  // 下一个command的id
  uint64_t                                     next_cmd_id_;
  // watch的管理器对象
  DcmdCenterWatchMgr*                          watches_;
};

}  // dcmd
#endif
