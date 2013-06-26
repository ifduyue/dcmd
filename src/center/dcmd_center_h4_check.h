#ifndef __DCMD_CENTER_H4_CHECK_H__
#define __DCMD_CENTER_H4_CHECK_H__
#include <CwxCommander.h>
#include "../cmn/dcmd_tss.h"
#include "dcmd_center_def.h"
#include "dcmd_mysql.h"

namespace dcmd {
class DcmdCenterApp;
// Master Center检查的Handler
class DcmdCenterH4Check: public CwxCmdOp {
 public:
  DcmdCenterH4Check(DcmdCenterApp* app):app_(app){ }
  virtual ~DcmdCenterH4Check(){}
 public:
   // 检测定时check
   virtual int onTimeoutCheck(CwxMsgBlock*& msg, CwxTss* pThrEnv);
   // 获取当前master的主机，若master_host为空，则表示没有master
   static bool GetMasterHost(Mysql* my, string& master_host, DcmdTss* tss);
 
 private:
   // lock table
   bool LockCenterTable(Mysql* my, DcmdTss* tss);
   // unlock table
   bool UnlockCenterTable(Mysql* my, DcmdTss* tss);
   // 设置心跳信息
   bool SetHeatbeat(Mysql* my, bool is_master, DcmdTss* tss);
   // 检测自己是否为master
   void CheckMasterCenter(DcmdTss* tss);
 private:
  // app对象
  DcmdCenterApp*            app_;
};
}  // dcmd
#endif 

