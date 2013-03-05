#ifndef __DCMD_TSS_H__
#define __DCMD_TSS_H__

#include <CwxLogger.h>
#include <CwxTss.h>

#include "dcmd_macro.h"

namespace dcmd {
//dcmd的tss
class DcmdTss:public CwxTss{
 public:
   const uint32_t kMaxSqlBufSize =  2 * 1024 * 1024; ///<SQL最大长度

 public:
  DcmdTss():CwxTss(){
    data_buf_ = NULL;
    data_buf_len_ = 0;
  }
  ///析构函数
  ~DcmdTss();
 
 public:
  ///tss的初始化，0：成功；-1：失败
  int Init();
  ///获取package的buf，返回NULL表示失败
  inline char* GetBuf(uint32_t buf_size){
    if (data_buf_len_ < buf_size){
      if (data_buf_) delete [] data_buf_;
      data_buf_ = new char[buf_size];
      data_buf_len_ = buf_size;
    }
    return data_buf_;
  }
   ///读取文件内容
  bool ReadFile(char const* filename, string& file_content,
    string& err_msg);
  char                  sql_[kMaxSqlBufSize]; ///<sql的buf
 private:
  char*                 data_buf_; ///<数据buf
  uint32_t              data_buf_len_; ///<数据buf的空间大小
};
}  // dcmd
#endif
