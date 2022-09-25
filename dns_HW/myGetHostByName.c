#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
// struct hostent * gethostbyname2(const char *hostname, int family);

int main()
{

    struct hostent *host = gethostbyname2("www.bilibili.com", AF_INET6);
    int i = 0;
    char buf[100];
    if (host != NULL)
    {
        // 官方域名
        printf("h_name = %s\r\n", host->h_name);

        // 别名
        for (i = 0; host->h_aliases[i]; i++)
        {
            printf("Aliases %d: %s\n", i + 1, host->h_aliases[i]);
        }

        // 地址类型
        printf("Address type: %s\n", (host->h_addrtype == AF_INET) ? "AF_INET" : "AF_INET6");

        // IP地址
        for (i = 0; host->h_addr_list[i]; i++)
        {
            printf("IP addr %d: %s\n", i + 1, inet_ntop(AF_INET6, host->h_addr_list[i], buf, 100));
        }
    }
    return 0;
}