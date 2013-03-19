#ifndef __DCMD_CENTER_DEF_H__
#define __DCMD_CENTER_DEF_H__

#include <CwxCommon.h>
#include <CwxHostInfo.h>

#include "../cmn/dcmd_def.h"
#include "../cmn/dcmd_tss.h"
#include "../cmn/dcmd.pb.h"
#include "../cmn/dcmd_macro.h"

namespace dcmd {
// 数据库cmd表的cmd 
class CenterTaskCmd{
 public:
  CenterTaskCmd(){
  }
 public:
 public:
  CWX_UINT32    m_uiMsgTaskId;  ///<消息对应的TaskId
  string		  m_strIp;        ///<指令对应的ip地址
  CWX_UINT32    m_uiTaskId;  ///<任务的id
  string   	   m_strTaskType;  ///<任务类型
  CWX_UINT64    m_ullTaskNodeId; ///<子任务的id
  CWX_UINT64    m_ullCmdId;     ///<指令的id
  bool		  m_bCtrl;      ///<是否为控制指令
  string       m_strSvrPool;  ///<指令对应的pool的名字
  string		  m_strSvr;      ///<指令对应的service的名字
  bool         m_bUpdateEnv; ///<是否更新service的环境
  bool         m_bUpdateTag; ///<是否更新版本库
  string       m_strTag;      ///<service的版本号
  string       m_strRepo;      ///<service的版本库地址
  string		  m_strSvrPath;   ///<service的路径
  string       m_strSvrUser;   ///<service的运行用户
  string       m_strEnvContent; ///<环境的配置内容
  string		  m_strEnvVersion;  ///<service的环境版本
  bool         m_bProcess;      ///<是否输出进度
  CWX_UINT32   m_uiTimeout;     ///<超时时间
  map<string, string> m_taskArgs; ///<任务的参数
  string       m_strScript;      ///<任务指令的执行脚本
  string       m_strProcess;     ///<当前进度信息
};

// agent的任务指令处理结果对象
class CenterTaskCmdResult {
public:
  ///构造函数
  DcmdTaskCmdResult(){
    m_uiMsgTaskId = 0;
    m_uiTaskId = 0;
    m_ullTaskNodeId = 0;
    m_ullCmdId = 0;
    m_bCtrl = false;
    m_bSuccess = true;
  }
public:
  ///根据notice对象，形成结果对象的部分信息
  DcmdTaskCmdResult& operator=(DcmdTaskCmd const& item){
    m_uiMsgTaskId = item.m_uiMsgTaskId;
    m_uiTaskId = item.m_uiTaskId;
    m_ullTaskNodeId = item.m_ullTaskNodeId;
    m_ullCmdId = item.m_ullCmdId;
    m_bCtrl = item.m_bCtrl;
    m_strProcess = item.m_strProcess;
    return *this;
  }
public:
  CWX_UINT32  m_uiMsgTaskId;  ///<消息对应的TaskId
  CWX_UINT32  m_uiTaskId;  ///<任务的id
  CWX_UINT64  m_ullTaskNodeId; ///<子任务的id
  CWX_UINT64  m_ullCmdId;     ///<指令的id
  bool		m_bCtrl;      ///<是否为控制指令
  bool        m_bSuccess;     ///<命令的执行状态
  string      m_strErrMsg;   ///<命令的错误消息
  string      m_strProcess; ///<进度信息
};


  class DcmdDbTask;
  class DcmdDbTaskNode;
  class DcmdNode;

  //class DcmdDbCmd，为数据库command表中的记录
  class DcmdDbCmd{
  public:
    ///命令类型定义
    enum{
      DCMD_CMD_TYPE_UNKNOWN  = 0,    ///<未知类型
      DCMD_CMD_TYPE_START_TASK = 1, ///<启动任务
      DCMD_CMD_TYPE_UPDATE_TASK = 2, ///<任务的参数发生了修改
      DCMD_CMD_TYPE_PAUSE_TASK = 3, ///<暂停任务的执行
      DCMD_CMD_TYPE_REDO_TASK = 4,  ///<重做任务的所有节点
      DCMD_CMD_TYPE_REDO_FAIL_TASK = 5, ///<重做任务中的失败节点
      DCMD_CMD_TYPE_FREEZE_TASK = 6, ///<冻结任务，任何状态都可冻结任务
      DCMD_CMD_TYPE_HISTORY_TASK = 7, ///<将任务的数据，搬移到history表
      DCMD_CMD_TYPE_REDO_SVR_POOL = 8, ///<重新执行池子的任务
      DCMD_CMD_TYPE_REDO_FAIL_SVR_POOL = 9, ///<重新执行池子中失败的任务
      DCMD_CMD_TYPE_ADD_TASK_NODE = 10, ///<增加了新节点任务
      DCMD_CMD_TYPE_EXEC_NODE = 11, ///<执行节点的任务
      DCMD_CMD_TYPE_REDO_NODE = 12, ///<重新执行节点的任务
      DCMD_CMD_TYPE_IGNORE_TASK_NODE = 13, ///<忽视节点上的task执行
      DCMD_CMD_TYPE_CANCEL_SVR_TASK = 14 ///<cancel agent的service任务
    };

    ///命令状态定义
    enum{
      DCMD_CMD_STATE_UNDO = 0, ///<命令未处理
      DCMD_CMD_STATE_FINISH = 1, ///<命令已经完成
      DCMD_CMD_STATE_FAIL = 2 ///<命令处理失败
    };

  public:
    ///构造函数
    DcmdDbCmd(){
      m_ullCmdId = 0;
      m_uiTaskId = 0;
      m_ullTaskNodeId = 0;
      m_bCtrl = false;
      m_uiCmdType = DCMD_CMD_TYPE_UNKNOWN;
      m_uiState = DCMD_CMD_STATE_UNDO;
      m_pCmd = NULL;
      m_pTaskNode = NULL;
      m_pNode = NULL;
    }
    ///析构函数
    ~DcmdDbCmd(){
      if (m_pCmd) delete m_pCmd;
      m_pCmd = NULL;
    }
  public:
    CWX_UINT64              m_ullCmdId; ///<指令的id
    CWX_UINT32              m_uiTaskId;  ///<命令的task id
    CWX_UINT64              m_ullTaskNodeId; ///<子任务的id
    bool                    m_bCtrl;        ///<是否为控制指令
    string                  m_strSvrPool;   ///<指令对应的池子
    string                  m_strSvr;   ///<指令对应的service
    string                  m_strIp;    ///<指令对应的Ip
    CWX_UINT32              m_uiCmdType; ///<指令的类型
    CWX_UINT32              m_uiState; ///<指令的状态
    DcmdTaskCmd*            m_pCmd; ///<命令对应的cmd对象
    DcmdDbTaskNode*         m_pTaskNode; ///<命令对应的tasknode，对于控制指令为空
    DcmdNode*               m_pNode;  ///<命令对应的node
  };


  ///任务设备对象，对应于svr_task_node表
  class DcmdDbTaskNode{
  public:
    ///任务Node的状态
    enum{
      DCMD_TASK_NSTATE_INIT = 0, ///<未执行
      DCMD_TASK_NSTATE_DOING = 1, ///<正在执行状态
      DCMD_TASK_NSTATE_FAIL =2,  ///<执行失败
      DCMD_TASK_NSTATE_FINISH =3, ///<已经完成
      DCMD_TASK_NSTATE_MAX = 3 ///<最大的状态值
    };
  public:
    ///构造函数
    DcmdDbTaskNode(){
      m_ullTaskNodeId = 0;
      m_uiTaskId = 0;
      m_uiState = DCMD_TASK_NSTATE_INIT;
      m_bIgnore = false;
      m_uiStartTime = 0;
      m_uiFinishTime = 0;
      m_cancelCmd = NULL;
      m_execCmd = NULL;
      m_pTask = NULL;
    }
  public:
    CWX_UINT64          m_ullTaskNodeId; ///<针对服务器的子任务id
    CWX_UINT32          m_uiTaskId; ///<任务ID
    string              m_strSvrPool; ///<service池子的名字
    string              m_strSvrName; ///<service的名字
    string              m_strIp; ///<服务器对应的ip地址
    CWX_UINT32          m_uiState; ///<子任务的状态
    bool                m_bIgnore; ///<是否忽视此节点的任务执行
    CWX_UINT32          m_uiStartTime; ///<任务的开始处理时间
    CWX_UINT32          m_uiFinishTime; ///<任务的完成处理时间
    DcmdDbCmd*          m_cancelCmd; ///<此服务对应的cancel指令。
    //但cancel一个execCmd时，exec不清空。但若再exec，则会覆盖先前的exec。
    DcmdDbCmd*          m_execCmd; ///<此服务对应的执行指令
    DcmdDbTask*         m_pTask; ///<子任务对应的task
  };


  //Task pool 对象，对应于task_svr_pool表
  class DcmdDbTaskSvrPool{
  public:
    ///构造函数
    DcmdDbTaskSvrPool(){
      m_uiTaskId = 0;
      m_uiTotalPoolHost = 0;
      m_uiTaskHostNum = 0;
      m_uiUndoHostNum = 0;
      m_uiDoingHostNum = 0;
      m_uiFinishHostNum = 0;
      m_uiIgnoreDoingHostNum =0;
      m_uiFailHostNum = 0;
      m_uiIgnoreFailHostNum = 0;
    }
    ///析构函数
    ~DcmdDbTaskSvrPool(){
      ///无需释放内部的对象，此对象放在DcmdCenterTaskMgr对象中管理、释放
      m_allTaskNodeMap.clear();
      m_undoTaskNodeMap.clear();
      m_doingTaskNodeMap.clear();
      m_finishTaskNodeMap.clear();
      m_failTaskNodeMap.clear();
    }
  public:
    ///往池子中添加tasknode；true：成功；false表示存在
    bool addTaskNode(DcmdDbTaskNode* pTaskNode);
    ///改变tasknnode的状态；true：成功；false表示不存在
    bool changeTaskNodeState(DcmdDbTaskNode const* pTaskNode, CWX_UINT32 uiNewState);
    ///ignore tasknode；true：成功；false表示不存在；
    bool ignoreTaskNode(DcmdDbTaskNode const* pTaskNode, bool bIgnore);
  public:
    ///是否可以调度
    inline bool enableSchedule(CWX_UINT32 uiDoingNum, ///<当前正在做的数量
      CWX_UINT32 uiContNum, ///<最大并发的数量
      CWX_UINT32 uiDoingRate ///<做的最大比率
      ) const
    {
      if (uiDoingNum >= uiContNum) return false;
      ///uiDoingNum加1的原因是计算若增加一台是否超过规定
      if ((uiDoingNum + 1) * 100 > m_uiTotalPoolHost * uiDoingRate) return false;
      return true;
    }
    ///获取池子node的数量
    CWX_UINT32 getTaskHostNum() const{
      return m_uiTaskHostNum;
    }
    ///获取未作node的数量
    CWX_UINT32 getUndoHostNum() const{
      return m_uiUndoHostNum;  ///<未作的设备数量
    }
    ///获取正在做的node的数量
    CWX_UINT32 getDoingHostNum() const{
      return m_uiDoingHostNum;  ///<正在做的设备数量
    }
    ///获取ignore的正在做的node的数量
    CWX_UINT32 getIgnoreDoingHostNum() const{
      return m_uiIgnoreDoingHostNum; ///<ignore状态的设备数量
    }
    ///获取已经完成的node的数量
    CWX_UINT32 getFinishHostNum() const{
      return m_uiFinishHostNum;
    }
    ///获取失败的node的数量 
    CWX_UINT32 getFailHostNum() const{
      return m_uiFailHostNum;   ///<失败的设备数量
    }
    ///获取ignore的失败node的数量
    CWX_UINT32 getIgnoreFailHostNum() const{
      m_uiIgnoreFailHostNum; ///<ignore状态的失败设备数量
    }
    ///获取所有的task node
    map<CWX_UINT64, DcmdDbTaskNode*> const* getAllTaskNode() const{
      return &m_allTaskNodeMap; ///<所有设备的tasknode map
    }
    ///获取未作的task node
    map<CWX_UINT64, DcmdDbTaskNode*> const* getUndoTaskNode() const{
      return &m_undoTaskNodeMap; ///<task上未作的tasknode map
    }
    ///获取正在做的task node
    map<CWX_UINT64, DcmdDbTaskNode*> const* getDoingTaskNode() const{
      return &m_doingTaskNodeMap; ///<task上正在做的tasknode map
    }
    ///获取已经完成的task node
    map<CWX_UINT64, DcmdDbTaskNode*> const* getFinishTaskNode() const{
      return &m_finishTaskNodeMap; ///<task上完成的tasknode map
    }
    ///获取失败的task node
    map<CWX_UINT64, DcmdDbTaskNode*> const* getFailTaskNode() const{
      return &m_failTaskNodeMap; ///<task上失败的tasknode map
    }

  public:
    CWX_UINT32          m_uiTaskId; ///<任务的id
    string              m_strSvrPool; ///<池子的名字
    string              m_strRepo;  ///<版本库地址
    string              m_strSvrUser; ///<service的用户
    string              m_strEnvVer; ///<service的环境版本号
    string              m_strEnv; ///<service池子的环境信息
    CWX_UINT32          m_uiTotalPoolHost; ///<池子中设备的数量
  private:
    CWX_UINT32          m_uiTaskHostNum;   ///<任务的设备数量
    CWX_UINT32          m_uiUndoHostNum;  ///<未作的设备数量
    CWX_UINT32          m_uiDoingHostNum;  ///<正在做的设备数量
    CWX_UINT32          m_uiIgnoreDoingHostNum; ///<ignore状态的设备数量
    CWX_UINT32          m_uiFinishHostNum; ///<已经完成的节点的数量
    CWX_UINT32          m_uiFailHostNum;   ///<失败的设备数量
    CWX_UINT32          m_uiIgnoreFailHostNum; ///<ignore状态的失败设备数量
    map<CWX_UINT64, DcmdDbTaskNode*> m_allTaskNodeMap; ///<所有设备的tasknode map
    map<CWX_UINT64, DcmdDbTaskNode*> m_undoTaskNodeMap; ///<task上未作的tasknode map
    map<CWX_UINT64, DcmdDbTaskNode*> m_doingTaskNodeMap; ///<task上正在做的tasknode map
    map<CWX_UINT64, DcmdDbTaskNode*> m_finishTaskNodeMap; ///<task上完成的tasknode map
    map<CWX_UINT64, DcmdDbTaskNode*> m_failTaskNodeMap; ///<task上失败的tasknode map
  };


  ///任务对象，对应数据库的svr_task表
  class DcmdDbTask{
  public:
    ///任务状态定义
    enum{
      TASK_STATE_INIT = 0, ///<未提交状态
      TASK_STATE_DOING = 1, ///<正在处理状态
      TASK_STATE_PAUSE = 2,  ///<暂停状态
      TASK_STATE_FREEZE = 3, ///<冻结状态
      TASK_STATE_MAX = 3 ///<状态的最大值
    };

  public:
    ///构造函数
    DcmdDbTask(){
      m_uiTaskId = 0;
      m_uiTimeout = 0;
      m_bAuto = false;
      m_bCheckout = false;
      m_bUpdateEnv = false;
      m_bUpdateTag = false;
      m_bProcess = false;
      m_uiContNum = 0;
      m_uiRate = 10; ///<10%的比率
      m_uiState = TASK_STATE_INIT;
      m_uiTaskHostNum = 0;
      m_uiUndoHostNum = 0;
      m_uiDoingHostNum = 0;
      m_uiIgnoreDoingHostNum =0;
      m_uiFinishHostNum = 0;
      m_uiFailHostNum = 0;
      m_uiIgnoreFailHostNum = 0;

    }

    ///析构函数
    ~DcmdDbTask(){
      ///需要释放池子对象
      map<string, DcmdDbTaskSvrPool*>::iterator iter= m_poolMap.begin();
      while(iter != m_poolMap.end()){
        delete iter->second;
        iter++;
      }
      m_poolMap.clear();
    }
  public:
    ///往task中添加service池子；true 成功；false：存在
    bool addSvrPool(DcmdDbTaskSvrPool* pool);
    ///获取task的池子
    DcmdDbTaskSvrPool* getSvrPool(string const& strPoolName);
    ///添加新任务，true 成功；false：失败。失败或者tasknode存在，或者pool不存在
    bool addTaskNode(DcmdDbTaskNode* pTaskNode);
    ///改变任务的状态，true 成功；false：失败。失败或者tasknode不存在，或者pool不存在
    bool changeTaskNodeState(DcmdDbTaskNode const* pTaskNode, CWX_UINT32 uiNewState);
    ///ignore tasknode；true：成功；false表示不存在；
    bool ignoreTaskNode(DcmdDbTaskNode const* pTaskNode, bool bIgnore);
    ///重新计算任务的数据统计。返回值：true，数据发生改变；false，数据没有改变
    bool checkTaskNodeStats();
  public:
    CWX_UINT32          m_uiTaskId; ///<任务ID
    string              m_strTaskName; ///<任务的名字
    string              m_strTaskType; ///<任务的类型
    string              m_strSvrName; ///<任务对应的service的名字
    string              m_strTag; ///<service的tag
    string              m_strSvrPath;  ///<service的路径
    CWX_UINT32          m_uiTimeout; ///<超时的时间
    bool                m_bCheckout; ///<是否强行取新版本
    bool                m_bAuto;      ///<是否自动执行
    bool                m_bUpdateEnv;  ///<是否更新环境信息
    bool                m_bUpdateTag;  ///<是否变更版本
    bool                m_bProcess;  ///<是否输出进度
    CWX_UINT32          m_uiContNum; ///<并行执行的数量
    CWX_UINT32          m_uiRate; ///<执行的最大比例
    CWX_UINT32          m_uiState; ///<任务的状态
    string              m_strScript; ///<任务的脚本，若为空表示错误
    string              m_strTaskArgs; ///<任务的脚本，若为空表示不存在
    string              m_strMd5;  ///<任务脚本的md5签名
    map<string, string> m_taskArgs; ///<任务的参数
    CWX_UINT32          m_uiTaskHostNum;   ///<任务的设备数量
    CWX_UINT32          m_uiUndoHostNum;  ///<未作的设备数量
    CWX_UINT32          m_uiDoingHostNum;  ///<正在做的设备数量
    CWX_UINT32          m_uiIgnoreDoingHostNum; ///<ignore状态的设备数量
    CWX_UINT32          m_uiFinishHostNum; ///<已经完成node节点的数量
    CWX_UINT32          m_uiFailHostNum;   ///<失败的设备数量
    CWX_UINT32          m_uiIgnoreFailHostNum; ///<ignore状态的失败设备数量
    map<string,DcmdDbTaskSvrPool*> m_poolMap; ///<任务的service pool map
  };

  ///agent节点对象，只有存在task命令的agent节点才存在
  class DcmdNode{
  public:
    ///构造函数
    DcmdNode(string const& strIp){
      m_strIp = strIp;
    }
    ///析构函数
    ~DcmdNode(){
      m_cmdMap.clear();
    }
  public:
    string      m_strIp;  ///设备的ip地址
    map<CWX_UINT64/*tasknode_id*/, DcmdDbCmd*>  m_cmdMap; ///<节点上待发送的命令
  };


  ///service状态对象
  class DcmdAgentSvrObj{
  public:
    DcmdAgentSvrObj(){
      m_svrProc = 0;
      m_pRunCmd = NULL;
    }
    ~DcmdAgentSvrObj(){
      m_cmds.clear();
      m_pRunCmd = NULL;
    }
  public:
    string							m_strSvr;  ///<service的名字
    list<DcmdTaskCmd*>	           m_cmds;    ///<service的命令
    pid_t							m_svrProc; ///<service的执行进程id
    DcmdTaskCmd*                  m_pRunCmd; ///<当前正在运行的指令，不在m_cmds中。
  };

}  // dcmd

#endif
