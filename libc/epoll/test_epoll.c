/***********************************************************************
*   Copyright (C) 2016 pilot-lab.inc All rights reserved.
*   
*   @file:       test_epoll.c
*   @brief:      
*   @author:     Pilot labs
*   @maintainer: frank.fu@pilot-lab.com.cn
*   @version:    1.0
*   @date:       2016-12-14
*   
***********************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <stdlib.h>

#define MAXLINE 5
#define OPEN_MAX 100
#define LISTENQ 20
#define SERV_PORT 5000
#define INFTIM 1000

void setnonblocking(int sock)
{
	int opts;
	opts=fcntl(sock,F_GETFL);
	if(opts<0)
	{
		perror("fcntl(sock,GETFL)");
		exit(1);
	}
	opts = opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<0)
	{
		perror("fcntl(sock,SETFL,opts)");
		exit(1);
	}
}

int main(int argc, char* argv[])
{
	int i, maxi, listenfd, connfd, sockfd,epfd,nfds, portnumber;
	ssize_t n;
	char line[MAXLINE];
	socklen_t clilen;

	if ( 2 == argc )
	{
		if( (portnumber = atoi(argv[1])) < 0 )
		{
			fprintf(stderr,"Usage:%s portnumber/a/n",argv[0]);
			return 1;
		}
	}
	else
	{
		fprintf(stderr,"Usage:%s portnumber/a/n",argv[0]);
		return 1;
	}

	//����epoll_event�ṹ��ı���,ev����ע���¼�,�������ڻش�Ҫ�������¼�
	struct epoll_event ev,events[20];
	//�������ڴ���accept��epollר�õ��ļ�������
	epfd=epoll_create(256);
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	//��socket����Ϊ��������ʽ

	setnonblocking(listenfd);

	//������Ҫ�������¼���ص��ļ�������

	ev.data.fd=listenfd;
	//����Ҫ�������¼�����

	ev.events=EPOLLIN|EPOLLET;
	//ev.events=EPOLLIN;

	//ע��epoll�¼�
	epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	char *local_addr="127.0.0.1";
	inet_aton(local_addr,&(serveraddr.sin_addr));//htons(portnumber);

	serveraddr.sin_port=htons(portnumber);
	bind(listenfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr));
	listen(listenfd, LISTENQ);
	maxi = 0;
	for ( ; ; ) {
		//�ȴ�epoll�¼��ķ���
		nfds=epoll_wait(epfd,events,20,500);

		//�����������������¼�
		for(i=0;i<nfds;++i)
		{
			printf("nfds=%d\n", nfds);
			if(events[i].data.fd==listenfd)//����¼�⵽һ��SOCKET�û����ӵ��˰󶨵�SOCKET�˿ڣ������µ����ӡ�
			{
				connfd = accept(listenfd,(struct sockaddr *)&clientaddr, &clilen);
				if(connfd<0){
					perror("connfd<0");
					exit(1);
				}
				//setnonblocking(connfd);

				char *str = inet_ntoa(clientaddr.sin_addr);
				printf("accapt a connection from %s, connfd=%d\n", str, connfd);
				//�������ڶ��������ļ�������
				ev.data.fd=connfd;
				//��������ע��Ķ������¼�
				ev.events=EPOLLIN|EPOLLET;
				//ev.events=EPOLLIN;
				//ע��ev
				epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
			}
			else if(events[i].events&EPOLLIN)//������Ѿ����ӵ��û��������յ����ݣ���ô���ж��롣
			{
				printf("EPOLLIN\n");
				if ( (sockfd = events[i].data.fd) < 0)
					continue;
				if ( (n = read(sockfd, line, MAXLINE)) < 0) {
					if (errno == ECONNRESET) {
						close(sockfd);
						events[i].data.fd = -1;
					} else
						printf("readline error\n");
				} else if (n == 0) {
					close(sockfd);
					events[i].data.fd = -1;
				}
				line[n] = '\0';
				printf("read:%s, fd=%d\n", line, sockfd);

				//��������д�������ļ�������
				ev.data.fd=sockfd;
				//��������ע���д�����¼�
				ev.events=EPOLLOUT|EPOLLET;
				//�޸�sockfd��Ҫ�������¼�ΪEPOLLOUT
				//epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
			}
			else if(events[i].events&EPOLLOUT) // ��������ݷ���
			{
				printf("EPOLLOUT\n");
				sockfd = events[i].data.fd;
				printf("write fd=%d\n", sockfd);
				write(sockfd, line, n);
				//�������ڶ��������ļ�������

				ev.data.fd=sockfd;
				//��������ע��Ķ������¼�

				ev.events=EPOLLIN|EPOLLET;
				//�޸�sockfd��Ҫ�������¼�ΪEPOLIN
				epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
			}
		}
	}

	return 0;
}