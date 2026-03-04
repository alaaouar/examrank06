#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

char rbuf[120000], wbuf[120000];
int fd_max = 0, next_id = 0;
fd_set rfds,wfds,fds;
char msg[1024][110000];
int cl_id[1100];

void fatal(void)
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void nnotify(int from)
{
    for (int fd = 0; fd <= fd_max; fd++)
        if (FD_ISSET(fd, &wfds) && fd != from)
            send(fd, wbuf, strlen(wbuf), 0);
}

int main(int ac, char **av)
{
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }
    FD_ZERO(&fds);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fatal();
    fd_max = sockfd;
    FD_SET(sockfd, &fds);
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433);
    addr.sin_port = htons(atoi(av[1]));
    socklen_t len = sizeof(addr);
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) || listen(sockfd, 128))
        fatal();
    while (1)
    {
        rfds = wfds =fds;
        if (select(fd_max +1 , &rfds , &wfds, NULL, NULL ) < 0)
            continue;
        for (int fd = 0; fd <= fd_max; fd++)
        {
            if (FD_ISSET(fd, &rfds) && fd == sockfd)
            {
                int client = accept(fd, (struct sockaddr *)&addr, &len);
                if (client < 0)
                    continue;
                FD_SET(client, &fds);
                fd_max = (client > fd_max) ? client : fd_max;
                cl_id[client] = next_id++;
                sprintf(wbuf, "server: client %d just arrived\n", cl_id[client]);
                nnotify(client);
                break;
            }
            if (FD_ISSET(fd, &rfds) && fd != sockfd)
            {
                int res = recv(fd, rbuf, sizeof(rbuf), 0);
                if (res <= 0)
                {
                    sprintf(wbuf, "server: client %d just left\n", cl_id[fd]);
                    nnotify(fd);
                    FD_CLR(fd, &fds);
                    bzero(msg[fd], strlen(msg[fd]));
                    close(fd);
                    break;
                }
                else
                {
                    for(int i = 0, j = strlen(msg[fd]); i < res; i++,j++)
                    {
                        msg[fd][j] = rbuf[i];
                        if (msg[fd][j] == '\n')
                        {
                            msg[fd][j] = '\0';
                            sprintf(wbuf, "client %d: %s\n", cl_id[fd], msg[fd]);
                            nnotify(fd);
                            bzero(msg[fd], strlen(msg[fd]));
                            j = -1;
                        }
                    }
                }
            }
        }
    }
}
