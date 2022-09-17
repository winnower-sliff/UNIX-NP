
/* traceroute.c */
#include "trace.h"

/*
 *  套接字地址结构地址部分转字符串
 *      若成功，返回转换后的字符串，若出错，返回NULL
 */
char *sock_ntop_host(const struct sockaddr *addr, socklen_t addrlen)
{
    static char str[64];

    switch (addr->sa_family)
    {
    case AF_INET:
    {
        struct sockaddr_in *sin = (struct sockaddr_in *)addr;
        if (inet_ntop(AF_INET, &sin->sin_addr,
                      str, sizeof(str)) == NULL)
            return NULL;
        break;
    }

    case AF_INET6:
    {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr;
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
            return NULL;
        break;
    }
    }

    return str;
}

/*
 *  比较两个套接字地址结构是否相同
 *      若相同，返回0
 */
int sock_addr_cmp(const struct sockaddr *sa1, const struct sockaddr *sa2, socklen_t salen)
{
    if (sa1->sa_family != sa2->sa_family)
        return -1;

    switch (sa1->sa_family)
    {
    case AF_INET:
        return memcmp(&((struct sockaddr_in *)sa1)->sin_addr,
                      &((struct sockaddr_in *)sa2)->sin_addr,
                      sizeof(struct in_addr));

    case AF_INET6:
        return memcmp(&((struct sockaddr_in6 *)sa1)->sin6_addr,
                      &((struct sockaddr_in6 *)sa2)->sin6_addr,
                      sizeof(struct in6_addr));
    }

    return -1;
}

/*
 *  计算检验和
 */
uint16_t in_cksum(uint16_t *addr, int len)
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

/*
 *  SIGALRM信号处理函数
 */
void sig_alrm(int signo)
{
    alarm_flag = TRUE;
    return;
}

void do_trace(void)
{
    switch (troptions.tro_type)
    {
    case TRACE_TYPE_TCP: //基于TCP
                         //          trace_TCP();
        break;

    case TRACE_TYPE_UDP: //基于UDP
        trace_udp();
        break;

    case TRACE_TYPE_ICMP: //基于ICMP
        trace_icmp();
        break;
    }
}

/*
 *  基于UDP的探测方法
 */
void trace_udp(void)
{
    int sendfd, recvfd;
    struct sockaddr addr, lastaddr;
    socklen_t addrlen;

    struct sockaddr_in myaddr;

    double rtt;
    int seq, ttl, query, code, ndone;
    struct timeval tvsend, tvrecv;

    /* 建立一个基于ICMP的套接字用于接收ICMP消息 */
    if ((recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        perror("scoket error");
        exit(EXIT_ERR);
    }
    setuid(getuid());

    /* 建立一个基于UDP的套接字用于发送探测数据 */
    if ((sendfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("soket error");
        exit(EXIT_ERR);
    }

    /* 绑定源端地址 */
    sport = (getpid() & 0xffff) | 0x8000;

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(sport);

    if (bind(sendfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) != 0)
    {
        perror("bind error");
        exit(EXIT_ERR);
    }

    printf("traceroute to %s (%s), %d hops max (UDP) \n",
           hostname,
           inet_ntoa(destaddr.sin_addr),
           troptions.tro_maxttl);

    seq = 0;
    ndone = 0;
    for (ttl = 1; ttl <= troptions.tro_maxttl && ndone == 0; ttl++)
    {
        setsockopt(sendfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

        printf("%3d", ttl);
        fflush(stdout);

        bzero(&lastaddr, sizeof(lastaddr));
        for (query = 0; query < troptions.tro_nqueries; query++)
        {
            ++seq;
            gettimeofday(&tvsend, NULL);

            destaddr.sin_port = htons(dport + seq);
            if (sendto(sendfd, sendbuf, datalen, 0, (struct sockaddr *)&destaddr, sizeof(destaddr)) < 0)
            {
                perror("sendto error");
                exit(EXIT_ERR);
            }

            /* 处理回应结果 */
            code = trace_recv_udp(recvfd, seq, &tvrecv, &addr, &addrlen);
            if (code == TRACE_RESULT_TIMEOUT)
            {
                printf("\t*");
            }
            else
            {
                char str[NI_MAXHOST];

                if (sock_addr_cmp(&lastaddr, &addr, addrlen) != 0)
                {
                    if (getnameinfo(&addr, addrlen, str, sizeof(str), NULL, 0, 0) == 0)
                        printf("\t%s (%s)",
                               str,
                               sock_ntop_host(&addr, addrlen));
                    else
                        printf("\t%s",
                               sock_ntop_host(&addr, addrlen));

                    memcpy(&lastaddr, &addr, addrlen);
                }

                if ((tvrecv.tv_usec -= tvsend.tv_usec) < 0)
                {
                    --tvrecv.tv_sec;
                    tvrecv.tv_usec += 1000000;
                }
                tvrecv.tv_sec -= tvsend.tv_sec;

                rtt = tvrecv.tv_sec * 1000.0 + tvrecv.tv_usec / 1000.0;

                printf("\t%.3f ms", rtt);

                if (code == TRACE_RESULT_UNREACH)
                    ndone++;
            }
        }

        printf("\n");
    }
}

int trace_recv_udp(int sockfd, int seq, struct timeval *tv, struct sockaddr *addr, socklen_t *addrlen)
{
    struct ip *ip1, *ip2;
    struct icmp *icmp;
    struct udphdr *udp;
    int iphlen1, iphlen2, icmplen, ret, n;
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_handler = sig_alrm;
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, NULL);

    alarm(troptions.tro_waittime);

    alarm_flag = FALSE;
    for (;;)
    {
        if (alarm_flag)
        {
            ret = TRACE_RESULT_TIMEOUT;
            break;
        }

        n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, addr, addrlen);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            else
            {
                perror("recv error");
                exit(EXIT_ERR);
            }
        }

        ip1 = (struct ip *)recvbuf;
        iphlen1 = ip1->ip_hl << 2;

        icmp = (struct icmp *)(recvbuf + iphlen1);
        if ((icmplen = n - iphlen1) < ICMP_HLEN)
            continue;

        if (icmp->icmp_type == ICMP_TIMXCEED && icmp->icmp_code == ICMP_TIMXCEED_INTRANS)
        {

            if (icmplen < ICMP_HLEN + sizeof(struct ip))
                continue;

            ip2 = (struct ip *)(recvbuf + iphlen1 + ICMP_HLEN);
            iphlen2 = ip2->ip_hl << 2;

            if (icmplen < ICMP_HLEN + iphlen2 + UDP_HLEN)
                continue;

            udp = (struct udphdr *)(recvbuf + iphlen1 + ICMP_HLEN + iphlen2);
            if (ip2->ip_p == IPPROTO_UDP && udp->source == htons(sport) && udp->dest == htons(dport + seq))
            {
                ret = TRACE_RESULT_TIMEEXCEED;
                break;
            }
        }
        else if (icmp->icmp_type == ICMP_UNREACH)
        {

            if (icmplen < ICMP_HLEN + sizeof(struct ip))
                continue;

            ip2 = (struct ip *)(recvbuf + iphlen1 + ICMP_HLEN);
            iphlen2 = ip2->ip_hl << 2;
            if (icmplen < ICMP_HLEN + iphlen2 + UDP_HLEN)
                continue;

            udp = (struct udphdr *)(recvbuf + iphlen1 + ICMP_HLEN + iphlen2);
            if (ip2->ip_p == IPPROTO_UDP && udp->source == htons(sport) && udp->dest == htons(dport + seq))
            {
                if (icmp->icmp_code == ICMP_UNREACH_PORT)
                    ret = TRACE_RESULT_UNREACH;
                else
                    ret = icmp->icmp_code; /* 0, 1, 2 ... */
                break;
            }
        }
    }
    alarm(0);
    gettimeofday(tv, NULL);

    return ret;
}

/*
 *  基于ICMP的探测方法
 */
void trace_icmp(void)
{
    int sockfd;
    struct sockaddr addr, lastaddr;
    socklen_t addrlen;

    double rtt;
    int icmpdatalen, seq, ttl, query, code, ndone;
    struct timeval tvsend, tvrecv;
    struct icmp *icmp;
    size_t len;

    /* 建立一个基于ICMP的套接字用于发送和接收ICMP消息 */
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        perror("scoket error");
        exit(EXIT_ERR);
    }
    setuid(getuid());

    printf("traceroute to %s (%s), %d hops max (ICMP) \n",
           hostname,
           inet_ntoa(destaddr.sin_addr),
           troptions.tro_maxttl);

    icmpdatalen = 56;
    seq = 0;
    ndone = 0;
    for (ttl = 1; ttl <= troptions.tro_maxttl && ndone == 0; ttl++)
    {
        setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

        printf("%3d", ttl);
        fflush(stdout);

        bzero(&lastaddr, sizeof(lastaddr));
        for (query = 0; query < troptions.tro_nqueries; query++)
        {
            ++seq;

            gettimeofday(&tvsend, NULL);

            /* 构建ICMP回显请求消息 */
            icmp = (struct icmp *)sendbuf;
            icmp->icmp_type = ICMP_ECHO;
            icmp->icmp_code = 0;
            icmp->icmp_id = htons(getpid());
            icmp->icmp_seq = htons(seq);
            memset(icmp->icmp_data, 0xa5, icmpdatalen);
            memcpy(icmp->icmp_data, &tvsend, sizeof(struct timeval));

            len = ICMP_HLEN + icmpdatalen;
            icmp->icmp_cksum = 0;
            icmp->icmp_cksum = in_cksum((u_short *)icmp, len);

            if (sendto(sockfd, sendbuf, len, 0, (struct sockaddr *)&destaddr, sizeof(destaddr)) < 0)
            {
                perror("sendto error");
                exit(EXIT_ERR);
            }

            /* 处理回应结果 */
            code = trace_recv_icmp(sockfd, seq, &tvrecv, &addr, &addrlen);
            if (code == TRACE_RESULT_TIMEOUT)
            {
                printf("\t*");
            }
            else
            {
                char str[NI_MAXHOST];

                if (sock_addr_cmp(&lastaddr, &addr, addrlen) != 0)
                {
                    if (getnameinfo(&addr, addrlen, str, sizeof(str), NULL, 0, 0) == 0)
                        printf("\t%s (%s)",
                               str,
                               sock_ntop_host(&addr, addrlen));
                    else
                        printf("\t%s",
                               sock_ntop_host(&addr, addrlen));

                    memcpy(&lastaddr, &addr, addrlen);
                }

                if ((tvrecv.tv_usec -= tvsend.tv_usec) < 0)
                {
                    --tvrecv.tv_sec;
                    tvrecv.tv_usec += 1000000;
                }
                tvrecv.tv_sec -= tvsend.tv_sec;

                rtt = tvrecv.tv_sec * 1000.0 + tvrecv.tv_usec / 1000.0;

                printf("\t%.3f ms", rtt);

                if (code == TRACE_RESULT_UNREACH)
                    ++ndone;
            }
        }

        printf("\n");
    }
}

int trace_recv_icmp(int sockfd, int seq, struct timeval *tv, struct sockaddr *addr, socklen_t *addrlen)
{
    struct ip *ip1, *ip2;
    struct icmp *icmp, *icmp2;
    int iphlen1, iphlen2, icmplen, ret, n;
    struct sigaction act;

    /* 设置SIGALRM信号处理函数 */
    sigemptyset(&act.sa_mask);
    act.sa_handler = sig_alrm;
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, NULL);

    alarm(troptions.tro_waittime);

    alarm_flag = FALSE;
    for (;;)
    {
        if (alarm_flag)
        {
            ret = TRACE_RESULT_TIMEOUT;
            break;
        }

        n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, addr, addrlen);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            else
            {
                perror("recv error");
                exit(EXIT_ERR);
            }
        }

        /* 处理接收到的数据 */
        ip1 = (struct ip *)recvbuf;
        iphlen1 = ip1->ip_hl << 2;

        icmp = (struct icmp *)(recvbuf + iphlen1);
        if ((icmplen = n - iphlen1) < ICMP_HLEN)
            continue;

        if (icmp->icmp_type == ICMP_TIMXCEED && icmp->icmp_code == ICMP_TIMXCEED_INTRANS)
        {

            if (icmplen < ICMP_HLEN + sizeof(struct ip))
                continue;

            ip2 = (struct ip *)(recvbuf + iphlen1 + ICMP_HLEN);
            iphlen2 = ip2->ip_hl << 2;

            if (icmplen < ICMP_HLEN + iphlen2 + ICMP_HLEN)
                continue;

            icmp2 = (struct icmp *)(recvbuf + iphlen1 + ICMP_HLEN + iphlen2);
            if (icmp2->icmp_type == ICMP_ECHO && icmp2->icmp_code == 0 && icmp2->icmp_id == htons(getpid()) && icmp2->icmp_seq == htons(seq))
            {
                ret = TRACE_RESULT_TIMEEXCEED;
                break;
            }
        }
        else if (icmp->icmp_type == ICMP_ECHOREPLY)
        {

            if (icmp->icmp_id == htons(getpid()) && icmp->icmp_seq == htons(seq))
            {
                ret = TRACE_RESULT_UNREACH;
                break;
            }
        }
    }
    alarm(0);
    gettimeofday(tv, NULL);

    return ret;
}
