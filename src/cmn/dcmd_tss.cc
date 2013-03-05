#include "dcmd_tss.h"

namespace dcmd {
///构造函数
DcmdTss::~DcmdTss(){
  if (data_buf_) delete []data_buf_;
}

///tss初始话
int DcmdTss::Init(){
  if (data_buf_) delete [] data_buf_;
  data_buf_ = NULL;
  data_buf_len_ = 0;
  return 0;
}

///读取文件内容
bool DcmdTss::readFile(char const* filename, string& file_content,
  string& err_msg){
  FILE* fd = fopen(filename, "rb");
  off_t file_size = CwxFile::getFileSize(filename);
  if (-1 == file_size){
    CwxCommon::snprintf(m_szBuf2K, 2048, "Failure to get file size, file:%s,
      errno=%d", filename, errno);
      err_msg = m_szBuf2K;
    return false;
  }
  if (!fd){
    CwxCommon::snprintf(m_szBuf2K, 2048, "Failure to open file:%s, errno=%d",
      filename, errno);
    err_msg = m_szBuf2K;
    return false;
  }
  char* szBuf = getBuf(file_size);
  if (file_size != fread(szBuf, 1, file_size, fd)){
    CwxCommon::snprintf(m_szBuf2K, 2048, "Failure to read file:%s, errno=%d",
      filename, errno);
    err_msg = m_szBuf2K;
    return false;
  }
  file_content.assign(szBuf, file_size);
  return true;
}

}