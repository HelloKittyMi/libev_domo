#include <ev.h>
#include <event.h>
//#include <ev++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <execinfo.h>
#include <dirent.h>

#include <fcntl.h>
#include <stdint.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#define MAXLEN 1023
#define PORT 8003
#define ADDR_IP "192.168.56.18"

/**************************************************************************************
ev_io的主要使命就是监听并响应指定文件描述fd上的读写事件。对fd的监听工作，主要委托给底层
的io库来完成。libev对目前比较流行的io库都提供了支持，如：select, epoll以及windows的iocp
等。在这里libev使用了Adaptor模式，通过统一的适配层隐藏了底层io库的细节。
****************************************************************************************/


int socket_init();
void accept_callback(struct ev_loop *loop, ev_io *w, int revents);
void recv_callback(struct ev_loop *loop, ev_io *w, int revents);
void write_callback(struct ev_loop *loop, ev_io *w, int revents);


int main(int argc ,char** argv)
{
    int listen;
    /**/
    ev_io ev_io_watcher;
    /*建立server*/
    listen=socket_init();
    
    /*新建事件监听loop*/
    struct ev_loop *loop = ev_loop_new(EVBACKEND_EPOLL);
    /*ev_io的主要使命就是监听并响应指定文件描述fd上的读写事件*/
    /*初始化事件监听*/
    ev_io_init(&ev_io_watcher, accept_callback,listen, EV_READ);
    /*开始IO监听*/
    ev_io_start(loop,&ev_io_watcher); 
    /*开始主循环*/
    ev_loop(loop,0);
    /*销毁事件循环*/
    ev_loop_destroy(loop);
    return 0;

}

int socket_init()
{
    struct sockaddr_in my_addr;
    int listener;
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    } 
    else
    {
        printf("SOCKET CREATE SUCCESS!\n");
    }
    //setnonblocking(listener);
    int so_reuseaddr=1;
    setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&so_reuseaddr,sizeof(so_reuseaddr));
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = inet_addr(ADDR_IP);
    /*绑定socket*/
    if (bind(listener, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))== -1) 
    {
        perror("bind error!\n");
        exit(1);
    } 
    else
    {
        printf("IP BIND SUCCESS,IP:%s\n",ADDR_IP);
    }
    /*开始监听*/
    if (listen(listener, 1024) == -1) 
    {
        perror("listen error!\n");
        exit(1);
    } 
    else
    {
        printf("LISTEN SUCCESS,PORT:%d\n",PORT);
    }
    return listener;
}

void accept_callback(struct ev_loop *loop, ev_io *w, int revents)
{
    int newfd;
    struct sockaddr_in sin;
    socklen_t addrlen = sizeof(struct sockaddr);
    ev_io* accept_watcher=malloc(sizeof(ev_io));
    while ((newfd = accept(w->fd, (struct sockaddr *)&sin, &addrlen)) < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            //these are transient, so don't log anything.
            continue; 
        }
        else
        {
            printf("accept error.[%s]\n", strerror(errno));
            break;
        }
    }
    /*新的watcher绑定事件*/
    ev_io_init(accept_watcher,recv_callback,newfd,EV_READ);
    /*子循环*/
    ev_io_start(loop,accept_watcher);
    printf("accept callback : fd :%d\n",accept_watcher->fd);

}

void recv_callback(struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[1024]={0};
    int ret =0;
    //ev_io write_event;
loop:
    ret=recv(w->fd,buffer,MAXLEN,0);
    if(ret > 0)
    {
        printf("recv message :%s  \n",buffer);
    }
    else if(ret ==0)
    {
        printf("remote socket closed!socket fd: %d\n",w->fd);
        close(w->fd);
        ev_io_stop(loop, w);
        free(w);
        return;
    }
    else
    {
        if(errno == EAGAIN ||errno == EWOULDBLOCK)
        {
            goto loop;
        }
        else
        {
            printf("ret :%d ,close socket fd : %d\n",ret,w->fd);
            close(w->fd);
            ev_io_stop(loop, w);
            free(w);
            return;
        }
    }
    int fd=w->fd;
    /*停止子循环*/
    ev_io_stop(loop,  w);
    /*绑定写回调*/
    ev_io_init(w,write_callback,fd,EV_WRITE);
    ev_io_start(loop,w);
    printf("socket fd : %d, turn read 2 write loop! ",fd);

}


void write_callback(struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[1024]={0};
    //ev_io read_event;
    snprintf(buffer,1023,"this is a libev server!\n");
    write(w->fd,buffer,strlen(buffer));
    int fd=w->fd;
    ev_io_stop(loop,  w);
    /*绑定读回调*/
    ev_io_init(w,recv_callback,fd,EV_READ);
    ev_io_start(loop,w);
}
