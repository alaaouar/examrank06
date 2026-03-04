#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

int fd_max = 0, next_id = 0;
fd_set rfds, wfds, fds;
char msg[1024][11000];
int cl_id[1024];
char rbuf[12000], wbuf[12000];

void fatalerr()                     {
    write(2,"Fatal error\n", 12);
    exit(1);
}

void notify(int sock)               {
    for(int fd = 0; fd <= fd_max; fd++)
        if (FD_ISSET(fd, &fds) && fd != sock)
            send(fd, wbuf, strlen(wbuf), 0);
}

int main(int ac, char **av)         {
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }
    FD_ZERO(&fds);
    bzero(msg, sizeof(msg));
    bzero(cl_id, sizeof(cl_id));
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fatalerr();
    fd_max = sockfd;
    FD_SET(sockfd, &fds);
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7F000001);
    addr.sin_port = htons(atoi(av[1]));
    socklen_t len = sizeof(addr);
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) || listen(sockfd, 128))
        fatalerr();
    while (1)
    {
        rfds = wfds = fds;
        if (select(fd_max + 1, &rfds, &wfds, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd <= fd_max; fd++)
        {
            if (FD_ISSET(fd, &rfds) && fd == sockfd)
            {
                int client = accept(fd, (struct sockaddr *)&addr, &len);
                if (client < 0)
                    continue;
                fd_max = (client > fd_max) ? client : fd_max;
                cl_id[client] = next_id++;
                FD_SET(client, &fds);
                sprintf(wbuf, "server : client %d has joined\n", cl_id[client]);
                notify(client);
                break;
            }
            if (FD_ISSET(fd, &rfds) && fd != sockfd)
            {
                int res = recv(fd, rbuf, sizeof(rbuf), 0);
                if (res <= 0)
                {
                    sprintf(wbuf, "server : client %d has left\n", cl_id[fd]);
                    notify(fd);
                    FD_CLR(fd, &fds);
                    cl_id[fd] = 0;
                    bzero(msg[fd], strlen(msg[fd]));
                    close(fd);
                    break;
                }
                else
                {
                    for (int i = 0, j = strlen(msg[fd]); i < res; i++,j++)
                    {
                        msg[fd][j] = rbuf[i];
                        if (msg[fd][j] == '\n')
                        {
                            msg[fd][j] = '\0';
                            sprintf(wbuf, "client %d: %s\n", cl_id[fd], msg[fd]);
                            notify(fd);
                            bzero(msg[fd], strlen(msg[fd]));
                            j = -1;
                        }
                    }
                    break;
                }
            }
        }
    }
    return 0;
}
