#include "commen.h"

int main()
{
    signal(SIGCHLD, sigHandlerForSigChild);
    signal(SIGPIPE, sigHandlerForSigPipe);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
        err_exit("socket error");
    int on = 1;
    if (setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,
                   &on,sizeof(on)) == -1)
        err_exit("setsockopt SO_REUSEADDR error");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8001);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenfd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
        err_exit("bind error");
    if (listen(listenfd, SOMAXCONN) == -1)
        err_exit("listen error");

    struct sockaddr_in clientAddr;
    socklen_t addrLen;
    int maxfd = listenfd;
    fd_set rset;
    fd_set allset;
    FD_ZERO(&rset);
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    //用于保存已连接的客户端套接字
    int client[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; ++i)
        client[i] = -1;
    int maxi = 0;   //用于保存最大的不空闲的位置, 用于select返回之后遍历数组

    while (true)
    {
        rset = allset;
        int nReady = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (nReady == -1)
        {
            if (errno == EINTR)
                continue;
            err_exit("select error");
        }
        //nReady == 0表示超时, 但是此处是一定不会发生的
        else if (nReady == 0)
            continue;

        if (FD_ISSET(listenfd, &rset))
        {
            addrLen = sizeof(clientAddr);
            int connfd = accept(listenfd, (struct sockaddr *)&clientAddr, &addrLen);
            if (connfd == -1)
                err_exit("accept error");

            int i;
            for (i = 0; i < FD_SETSIZE; ++i)
            {
                if (client[i] < 0)
                {
                    client[i] = connfd;
                    if (i > maxi)
                        maxi = i;
                    break;
                }
            }
            if (i == FD_SETSIZE)
            {
                cerr << "too many clients" << endl;
                exit(EXIT_FAILURE);
            }
            //打印客户IP地址与端口号
            cout << "Client information: " << inet_ntoa(clientAddr.sin_addr)
                 << ", " << ntohs(clientAddr.sin_port) << endl;
            //将连接套接口放入allset, 并更新maxfd
            FD_SET(connfd, &allset);
            if (connfd > maxfd)
                maxfd = connfd;

            if (--nReady <= 0)
                continue;
        }

        /**如果是已连接套接口发生了可读事件**/
        for (int i = 0; i <= maxi; ++i)
            if ((client[i] != -1) && FD_ISSET(client[i], &rset))
            {
                char buf[512] = {0};
                int readBytes = readline(client[i], buf, sizeof(buf));
                if (readBytes == -1)
                    err_exit("readline error");
                else if (readBytes == 0)
                {
                    cerr << "client connect closed..." << endl;
                    FD_CLR(client[i], &allset);
                    close(client[i]);
                    client[i] = -1;
                }
                //注意此处: Server从Client获取数据之后并没有立即回射回去,
                //        而是等待四秒钟之后再进行回射
                sleep(4);
                cout << buf;
                if (writen(client[i], buf, readBytes) == -1)
                    err_exit("writen error");

                if (--nReady <= 0)
                    break;
            }
    }
    close(listenfd);
}
