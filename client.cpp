#include "commen.h"

void echoClient(int sockfd);
int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        err_exit("socket error");

    //填写服务器端口号与IP地址
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8001);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
        err_exit("connect error");

    echoClient(sockfd);
    close(sockfd);
}
void echoClient(int sockfd)
{
    char buf[512];
    fd_set rset;
    //确保标准输入不会被重定向
    int fd_stdin = fileno(stdin);
    int maxfd = fd_stdin > sockfd ? fd_stdin : sockfd;
    bool stdineof = false;
    while (true)
    {
        FD_ZERO(&rset);
        if (!stdineof)
            FD_SET(fd_stdin, &rset);
        FD_SET(sockfd, &rset);
        int nReady = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (nReady == -1)
            err_exit("select error");
        else if (nReady == 0)
            continue;

        /** nReady > 0: 检测到了可读事件 **/

        if (FD_ISSET(fd_stdin, &rset))
        {
            memset(buf, 0, sizeof(buf));
            if (fgets(buf, sizeof(buf), stdin) == NULL)
            {
                /** 改用shutdown **/
                shutdown(sockfd, SHUT_WR);
                stdineof = true;
                /*
                close(sockfd);
                sleep(15);
                exit(EXIT_FAILURE);
                */
            }
            if (writen(sockfd, buf, strlen(buf)) == -1)
                err_exit("write socket error");
        }
        if (FD_ISSET(sockfd, &rset))
        {
            memset(buf, 0, sizeof(buf));
            int readBytes = readline(sockfd, buf, sizeof(buf));
            if (readBytes == 0)
            {
                cerr << "server connect closed..." << endl;
                exit(EXIT_FAILURE);
            }
            else if (readBytes == -1)
                err_exit("read-line socket error");

            cout << buf;
        }
    }
}
