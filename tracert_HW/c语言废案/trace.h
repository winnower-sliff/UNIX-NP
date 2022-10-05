
/* traceroute.h */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>

/* trace_recv_xxx函数返回值 */
#define TRACE_RESULT_TIMEOUT -3
#define TRACE_RESULT_TIMEEXCEED -2
#define TRACE_RESULT_UNREACH -1

/* 探测报文类型 */
#define TRACE_TYPE_UDP 1
#define TRACE_TYPE_TCP 2
#define TRACE_TYPE_ICMP 3

#define EXIT_ERR 1

#define BUFSIZE 1500
#define ICMP_HLEN 8
#define UDP_HLEN 8
#define TRUE 1
#define FALSE 0

/* troptions默认值 */
#define DFLOPT_MAXTTL 30
#define DFLOPT_NQUERIES 3
#define DFLOPT_WAITTIME 3
#define DFLOPT_TYPE TRACE_TYPE_UDP

#define USAGE_NEWLINE "\n"

/* troptions结构定义 */
struct troptions
{
    int tro_maxttl;   /* -m */
    int tro_nqueries; /* -q */
    int tro_waittime; /* -w */
    int tro_type;     /* -I -U -T */
};

/* 全局变量 */
extern struct troptions troptions;

extern uint16_t sport, dport;       //源端端口和目的端端口
extern struct sockaddr_in destaddr; //目的端套接字地址结构
extern char *hostname;              //目的端主机名

extern char sendbuf[BUFSIZE], recvbuf[BUFSIZE]; //发送缓冲区和接收缓冲区
extern int datalen;

extern int alarm_flag; //闹钟标记

/******************** 函数声明（开始） ********************/

/*
 *  打印用法信息
 */
void usage(void);

void do_trace(void);

/*
 *  基于UDP的探测函数
 */
void trace_udp(void);
int trace_recv_udp(int sockfd, int seq, struct timeval *tv, struct sockaddr *addr, socklen_t *addrlen);

/*
 *  基于ICMP的探测函数
 */
void trace_icmp(void);
int trace_recv_icmp(int sockfd, int seq, struct timeval *tv, struct sockaddr *addr, socklen_t *addrlen);

/*
 *  计算校验和
 */
uint16_t in_cksum(uint16_t *addr, int len);

/*
 *  将套接字地址的IP地址转换成对应的字符串
 */
char *sock_ntop_host(const struct sockaddr *addr, socklen_t addrlen);

/*
 *  比较两个套接字地址结构
 */
int sock_addr_cmp(const struct sockaddr *sa1, const struct sockaddr *sa2, socklen_t salen);

/******************** 函数声明（结束） ********************/