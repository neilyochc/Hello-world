#ifndef COMMEN_H_INCLUDED
#define COMMEN_H_INCLUDED

#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/time.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <iostream>
#include <algorithm>
using namespace std;

void sigHandlerForSigChild(int signo)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}
void sigHandlerForSigPipe(int signo)
{
    cerr << "recv a signal " << signo << ": " << strsignal(signo) << endl;
}

/**返回值说明:
    == count: 说明正确返回, 已经真正读取了count个字节
    == -1   : 读取出错返回
    <  count: 读取到了末尾
**/
inline ssize_t readn(int fd, void *buf, size_t count)
{
    size_t nLeft = count;
    ssize_t nRead = 0;
    char *pBuf = (char *)buf;
    while (nLeft > 0)
    {
        if ((nRead = read(fd, pBuf, nLeft)) < 0)
        {
            //如果读取操作是被信号打断了, 则说明还可以继续读
            if (errno == EINTR)
                continue;
            //否则就是其他错误
            else
                return -1;
        }
        //读取到末尾
        else if (nRead == 0)
            return count-nLeft;

        //正常读取
        nLeft -= nRead;
        pBuf += nRead;
    }
    return count;
}
/**返回值说明:
    == count: 说明正确返回, 已经真正写入了count个字节
    == -1   : 写入出错返回
**/
inline ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t nLeft = count;
    ssize_t nWritten = 0;
    char *pBuf = (char *)buf;
    while (nLeft > 0)
    {
        if ((nWritten = write(fd, pBuf, nLeft)) < 0)
        {
            //如果写入操作是被信号打断了, 则说明还可以继续写入
            if (errno == EINTR)
                continue;
            //否则就是其他错误
            else
                return -1;
        }
        //如果 ==0则说明是什么也没写入, 可以继续写
        else if (nWritten == 0)
            continue;

        //正常写入
        nLeft -= nWritten;
        pBuf += nWritten;
    }
    return count;
}

inline void err_exit(const std::string &msg)
{
    perror(msg.c_str());
    exit(EXIT_FAILURE);
}
inline void err_quit(const std::string &msg)
{
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
    while (true)
    {
        int ret = recv(sockfd, buf, len, MSG_PEEK);
        //如果recv是由于被信号打断, 则需要继续(continue)查看
        if (ret == -1 && errno == EINTR)
            continue;
        return ret;
    }
}
/** 返回值说明:
    == 0:   对端关闭
    == -1:  读取出错
    其他:    一行的字节数(包含'\n')
**/
ssize_t readline(int sockfd, void *buf, size_t maxline)
{
    int ret;
    int nRead = 0;
    int returnCount = 0;
    char *pBuf = (char *)buf;
    int nLeft = maxline;
    while (true)
    {
        ret = recv_peek(sockfd, pBuf, nLeft);
        //如果查看失败或者对端关闭, 则直接返回
        if (ret <= 0)
            return ret;
        nRead = ret;
        for (int i = 0; i < nRead; ++i)
            //在当前查看的这段缓冲区中含有'\n', 则说明已经可以读取一行了
            if (pBuf[i] == '\n')
            {
                //则将缓冲区内容读出
                //注意是i+1: 将'\n'也读出
                ret = readn(sockfd, pBuf, i+1);
                if (ret != i+1)
                    exit(EXIT_FAILURE);
                return ret + returnCount;
            }

        // 如果在查看的这段消息中没有发现'\n', 则说明还不满足一条消息,
        // 在将这段消息从缓冲中读出之后, 还需要继续查看
        ret = readn(sockfd, pBuf, nRead);;
        if (ret != nRead)
            exit(EXIT_FAILURE);
        pBuf += nRead;
        nLeft -= nRead;
        returnCount += nRead;
    }
    //如果程序能够走到这里, 则说明是出错了
    return -1;
}
#endif // COMMEN_H_INCLUDED
