#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
//#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <unistd.h>
//#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <malloc.h>
//#include <stdlib.h>
//#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <asm/byteorder.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BYTE u_char
#define DWORD u_long
#define USHORT u_short

// IP报头
typedef struct IP_HEADER
{
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8 ihl : 4,    // 4位头部长度 一位4个字节，，最多15*4个字节(可选项)
        version : 4; // 4位版本号
#elif defined(__BIG_ENDIAN_BITFIELD)
    __u8 version : 4,
        ihl : 4;
#else
#error "Please fix <asm/byteorder.h>"
#endif
    __u8 tos;        // 8位服务类型
    __be16 tot_len;  // 16位总长度
    __be16 id;       // 16位标识符
    __be16 frag_off; // 3位标志加13位片偏移
    __u8 ttl;        // 8位生存时间
    __u8 protocol;   // 8位上层协议号
    __sum16 check;   // 16位校验和
    __be32 saddr;    // 32位源IP地址
    __be32 daddr;    // 32位目的IP地址
    /*The options start here. */
} IP_HEADER;

// ICMP报头
typedef struct ICMP_HEADER
{
    u_char type;   // 8位类型字段
    u_char code;   // 8位代码字段
    u_short cksum; // 16位校验和
    u_short id;    // 16位标识符
    u_short seq;   // 16位序列号
} ICMP_HEADER;

//计算网际校验和函数
u_short checksum(u_short *pBuf, int iSize)
{
    unsigned long cksum = 0;
    while (iSize > 1)
    {
        cksum += *pBuf++;
        iSize -= sizeof(u_short);
    }
    if (iSize) //如果 iSize 为正，即为奇数个字节
    {
        cksum += *(u_char *)pBuf; //则在末尾补上一个字节，使之有偶数个字节
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (u_short)(~cksum);
}

/* This function will be invoked by pcap for each captured packet.
We can process each packet inside the function.
*/
// void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
// {
//     printf("Got a packet.\n");
//     sendSpoof(packet);
// }

// void sendSpoof(const u_char *packet)
void sendSpoof()
{
    int sd;
    struct sockaddr_in sin;
    char buffer[1024]; // You can change the buffer size
    /* Create a raw socket with IP protocol. The IPPROTO_RAW parameter
     * tells the sytem that the IP header is already included;
     * this prevents the OS from adding another IP header. */

    //构造ICMP回显请求消息，并以TTL递增的顺序发送报文
    // ICMP类型字段
    const BYTE ICMP_ECHO_REQUEST = 8; //请求回显
    const BYTE ICMP_ECHO_REPLY = 0;   //回显应答
    const BYTE ICMP_TIMEOUT = 11;     //传输超时

    //其他常量定义
    const int DEF_ICMP_DATA_SIZE = 32;     // ICMP报文默认数据字段长度
    const int MAX_ICMP_PACKET_SIZE = 1024; // ICMP报文最大长度（包括报头）
    const DWORD DEF_ICMP_TIMEOUT = 3000;   //回显应答超时时间
    const int DEF_MAX_HOP = 30;            //最大跳站数

    //填充ICMP报文中每次发送时不变的字段
    char *IcmpSendBuf = buffer + sizeof(IP_HEADER); //发送缓冲区
    memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf));    //初始化发送缓冲区
    // char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];                    //接收缓冲区
    // memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf));               //初始化接收缓冲区

    /*填写ICMP头，回显请求*/
    ICMP_HEADER *pIcmpHeader = (ICMP_HEADER *)IcmpSendBuf;
    pIcmpHeader->type = ICMP_ECHO_REQUEST;
    pIcmpHeader->code = 0;
    /*ID字段为当前进程号*/
    // pIcmpHeader->id = (USHORT)GetCurrentProcessId();
    pIcmpHeader->id = 0x18bb;
    memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'r', DEF_ICMP_DATA_SIZE); //数据字段

    //填充ICMP报文中每次发送变化的字段
    ((ICMP_HEADER *)IcmpSendBuf)->cksum = 0; //校验和先置为0
    //((ICMP_HEADER*)IcmpSendBuf)->seq = htons(usSeqNo++);      //填充序列号
    ((ICMP_HEADER *)IcmpSendBuf)->seq = 256;
    ((ICMP_HEADER *)IcmpSendBuf)->cksum = checksum((USHORT *)IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE); //计算校验和
    // printf("%d$$$$$$$$$$\n",sizeof(char));
    IP_HEADER *pIPHeader = (IP_HEADER *)buffer;

    pIPHeader->version = 4;
    pIPHeader->ihl = 5;
    pIPHeader->tos = 0;
    pIPHeader->tot_len = (sizeof(IP_HEADER) + sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE);
    pIPHeader->id = 1;
    pIPHeader->frag_off = 0x000;
    pIPHeader->ttl = 64;
    pIPHeader->protocol = 1; // TCP的协议号为6，UDP的协议号为17。ICMP的协议号为1，IGMP的协议号为2
    pIPHeader->saddr = inet_addr("172.25.187.134");
    // pIPHeader->saddr = getifaddrs("eth0");
    // pIPHeader->daddr = inet_addr("121.194.14.142");
    pIPHeader->daddr = ((struct in_addr *)(gethostbyname("www.bilibili.com")->h_addr))->s_addr;
    // memcpy(&pIPHeader->saddr, packet + 30, 4);
    // memcpy(&pIPHeader->daddr, packet + 26, 4);

    pIPHeader->check = checksum((USHORT *)buffer, sizeof(IP_HEADER) + sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE);

    sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sd < 0)
    {
        perror("socket() error");
        exit(-1);
    }
    /* This data structure is needed when sending the packets
     * using sockets. Normally, we need to fill out several
     * fields, but for raw sockets, we only need to fill out
     * this one field */
    sin.sin_family = AF_INET;
    // Here you can construct the IP packet using buffer[]
    // - construct the IP header ...
    // - construct the TCP/UDP/ICMP header ...
    // - fill in the data part if needed ...
    // Note: you should pay attention to the network/host byte order.
    /* Send out the IP packet.
     * ip_len is the actual size of the packet. */
    int ip_len = (sizeof(IP_HEADER) + sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE);
    if (sendto(sd, buffer, ip_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("sendto() error");
        exit(-1);
    }
    else
    {
        printf("SEND OUT!!!%d\n", pIPHeader->tot_len);
    }
}

int main()
{
    sendSpoof();
    return 0;
}
// Note: don’t forget to add "-lpcap" to the compilation command.
// For example: gcc -o sniff sniff.c -lpcap