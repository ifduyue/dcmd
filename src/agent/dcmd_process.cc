#include "dcmd_process.h"

#include <errno.h>
#include <stdio.h>

// 此类的使用方式是针对一个exe创建一个process的实例，然后指定不同的参数
// 环境变量运行。
// 可以通过Wait等待进程的结束，也可以通过TryWait轮询等待进程的结束。
namespace dcmd {
bool DcmdProcess::Run(list<string> const& process_arg,
    list<string> const& process_env, char* err_2k) {
  if (IsRuning()) return true;
  pid_t pid = -1;
  uint32_t index = 0;
  char const* sz_cmd_ptr = exec_file_.c_str();
  if (process_args_) delete [] process_args_;
  if (process_envs_) delete [] process_envs_;
  process_args_ = new char*[process_arg.size() + 2];
  process_envs_ = new char*[process_env.size() + 1];
  list<string>::const_iterator iter = process_arg.begin();
  // 设置命令参数
  process_args_[index++] = const_cast<char*>(sz_cmd_ptr);
  while (iter != process_arg.end()) {
    process_args_[index++] = const_cast<char*>(iter->c_str());
    ++iter;
  }
  process_args_[index] = NULL;
  // 设置环境变量
  iter = process_env.begin();
  index = 0;
  while (iter != process_env.end()) {
    process_envs_[index++] = const_cast<char*>(iter->c_str());
    ++iter;
  }
  process_envs_[index] = NULL;
  pid = ::vfork();
  if (-1 == pid) {
    snprintf(err_2k, kDcmd2kBufLen, "Failure to fork process, errno=%d", errno);
    return false;
  }
  if (pid) {
    //父进程
    pid_ = pid;
    start_time_ = time(NULL);
    status_ = 0;
    return true;
  }
  //子进程，并将进程设置为首进程
  if (-1 == setsid()) _exit(127);
  execve(sz_cmd_ptr, process_args_, process_envs_);
  // 若exec执行失败，则返回1
  _exit(127);
}

void DcmdProcess::Kill(bool is_kill_child) {
  int num_ret = 0;
  int status = 0;
  // 如果没有运行则直接退出
  if (!IsRuning())  return;
  // 采用double kill。首先是SIGTERM，然后是SIGKILL
  // 首先wait确定是否为自己的child
  num_ret = ::waitpid(pid_, &status, WNOHANG);
  if (0 == num_ret) {// 进程是孩子而且还活着
    pid_t killed_pid = is_kill_child ? -pid_ : pid_;
    // kill
    ::kill(killed_pid, SIGTERM);
    num_ret = ::waitpid(pid_, &status, WNOHANG);
    if (0 == num_ret) {
      ::kill(killed_pid, SIGKILL);
      ::waitpid(pid_, &status, WNOHANG);
    }
  }
  pid_ = -1;
  start_time_ = 0;
  status_ = 0;
}

// 返回值：-1：wait失败；1进程正常退出；2：进程异常退出
int DcmdProcess::Wait(char* err_2k) {
  while (waitpid(pid_, &status_, 0) < 0) {
    if (errno != EINTR) {
      status_ = -1;
      snprintf(err_2k, kDcmd2kBufLen, "failure to wait, errno=%d", errno);
      return -1;
    }
  }
  if (!WIFEXITED(status_)) return 2;
  return 1;
}

//返回值：-1：wait失败；0：进程还在运行；1：进程正常退出；2：进程异常退出
int DcmdProcess::TryWait(char* err_2k) {
  int num_ret = waitpid(pid_, &status_, WNOHANG);
  if (0 > num_ret) {
    if (errno == EINTR) return 0;
    status_ = -1;
    snprintf(err_2k, kDcmd2kBufLen, "failure to wait, errno=%d", errno);
    return -1;
  } else if (0 == num_ret) {
    return 0;
  }
  if (!WIFEXITED(status_)) return 2;
  return 1;
}

bool DcmdProcess::IsRuning() const {
  return -1 != pid_;
}
}

