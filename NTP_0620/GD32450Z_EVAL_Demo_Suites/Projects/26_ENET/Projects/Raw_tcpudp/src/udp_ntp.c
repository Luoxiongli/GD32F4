#include "udp_ntp.h"

uint8_t udp_ntp_dns_result[IP_LEN];
const char g_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
NTPformat g_ntpformat;                            /* NTP数据包结构体 */
DateTime g_nowdate;                               /* 时间结构体 */
uint8_t g_ntp_message[UDP_NTP_TX_BUFFSIZE];       /* NTP数据包缓存区 */
char g_ntp_message_sendbuff[UDP_NTP_TX_BUFFSIZE]; /* NTP发送数据包 */
uint8_t g_ntp_demo_recvbuf[UDP_NTP_RX_BUFFSIZE];  /* NTP接收数据缓冲区 */

uint8_t dns_flag = 0;
struct udp_pcb *udp_ntp_pcb; /* 定义一个UDP NTP控制块 */

static void ntp_dns_cbk(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    memset(udp_ntp_dns_result, 0, sizeof(udp_ntp_dns_result));
    dns_flag = 1;
    udp_ntp_dns_result[0] = (ipaddr->addr) >> 24;
    udp_ntp_dns_result[1] = (ipaddr->addr) >> 16;
    udp_ntp_dns_result[2] = (ipaddr->addr) >> 8;
    udp_ntp_dns_result[3] = (ipaddr->addr);
    udp_ntp_pcb = udp_new();
#if DNS_TEST
    printf("%s ip is: %d.%d.%d.%d\r\n", name, udp_ntp_dns_result[3], udp_ntp_dns_result[2], udp_ntp_dns_result[1], udp_ntp_dns_result[0]);
#endif
}

void udp_ntp_dns_get_ip(void)
{
    ip_addr_t ntp_server_ip;
    err_t err;
    err = dns_gethostbyname(ALIYUN_NTP_HOSTNAME, &ntp_server_ip, ntp_dns_cbk, NULL);
#if DNS_TEST
    if (err == ERR_OK)
    {
        printf("get host by name ERR_OK.\r\n");
    }
    else
    {
        printf("get host by name err:%d.\r\n", err);
    }
#endif
}

/*
 * 以UDP协议连接阿里云NTP服务器；
 * 发送NTP报文到阿里云NTP服务器；
 * 获取阿里云NTP 服务器返回的数据，取第40位到43位的十六进制数值；
 * 把40位到43位的十六进制数值转成十进制；
 * 把十进制数值减去1900-1970的时间差；
 * 数值转成年月日时分秒。
*/
static void udp_ntp_calc_time(unsigned long long time)
{
    unsigned int Pass4year;
    int hours_per_year;
    if (time <= 0)
    {
        time = 0;
    }

    g_nowdate.second = (int)(time % 60); /* 取秒时间 */
    time /= 60;

    g_nowdate.minute = (int)(time % 60); /* 取分钟时间 */
    time /= 60;

    g_nowdate.hour = (int)(time % 24); /* 小时数 */

    Pass4year = ((unsigned int)time / (1461L * 24L)); /* 取过去多少个四年，每四年有 1461*24 小时 */

    g_nowdate.year = (Pass4year << 2) + 1970; /* 计算年份 */

    time %= 1461 * 24; /* 四年中剩下的小时数 */

    for (;;) /* 校正闰年影响的年份，计算一年中剩下的小时数 */
    {
        hours_per_year = 365 * 24;
        if ((g_nowdate.year & 3) == 0)
        {

            hours_per_year += 24;
        }
        if (time < hours_per_year)
        {
            break;
        }
        g_nowdate.year++;
        time -= hours_per_year;
    }

    time /= 24;                    /* 一年中剩下的天数 */
    time++;                        /* 假定为闰年 */
    if ((g_nowdate.year & 3) == 0) /* 校正闰年的误差 */
    {
        if (time > 60)
        {
            time--;
        }
        else
        {
            if (time == 60)
            {
                g_nowdate.month = 1;
                g_nowdate.day = 29;
                return;
            }
        }
    }
    for (g_nowdate.month = 0; g_days[g_nowdate.month] < time; g_nowdate.month++) /* 计算月日 */
    {
        time -= g_days[g_nowdate.month];
    }

    g_nowdate.day = (int)(time);

    return;
}

static void udp_ntp_get_time(uint8_t *pbuf, uint16_t idx)
{
    unsigned long long atk_seconds = 0;
    uint8_t i = 0;

    for (i = 0; i < 4; i++) /* 获取40~43位的数据 */
    {
        atk_seconds = (atk_seconds << 8) | pbuf[idx + i]; /* 进制转换 */
    }

    atk_seconds -= TIMESTAMP_DELTA; /* 减去时间差 */
    udp_ntp_calc_time(atk_seconds); /* 由UTC时间计算日期 */
}

static void udp_ntp_message_init(void)
{
    uint8_t flag;
    uint8_t i;

    g_ntpformat.leap = 0;      /* leap indicator */
    g_ntpformat.version = 3;   /* version number */
    g_ntpformat.mode = 3;      /* mode */
    g_ntpformat.stratum = 0;   /* stratum */
    g_ntpformat.poll = 0;      /* poll interval */
    g_ntpformat.precision = 0; /* precision */
    g_ntpformat.rootdelay = 0; /* root delay */
    g_ntpformat.rootdisp = 0;  /* root dispersion */
    g_ntpformat.refid = 0;     /* reference ID */
    g_ntpformat.reftime = 0;   /* reference time */
    g_ntpformat.org = 0;       /* origin timestamp */
    g_ntpformat.rec = 0;       /* receive timestamp */
    g_ntpformat.xmt = 0;       /* transmit timestamp */

    flag = (g_ntpformat.version << 3) + g_ntpformat.mode; /* one byte Flag */
    memcpy(g_ntp_message, (void const *)(&flag), 1);

    for (i = 0; i < UDP_NTP_TX_BUFFSIZE; i++)
    {
        g_ntp_message_sendbuff[i] = (char)g_ntp_message[i];
        // snprintf(&g_ntp_message_sendbuff[2 * i], 3, "%02x", g_ntp_message[i]);
    }
}

static void udp_ntp_recv_cbk(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    uint32_t data_len = 0;
    struct pbuf *q;

    if (p != NULL) /* 接收到不为空的数据时 */
    {
        memset(g_ntp_demo_recvbuf, 0, UDP_NTP_RX_BUFFSIZE); /* 数据接收缓冲区清零 */

        for (q = p; q != NULL; q = q->next) /* 遍历完整个pbuf链表 */
        {
            /* 判断要拷贝到LWIP_DEMO_RX_BUFSIZE中的数据是否大于LWIP_DEMO_RX_BUFSIZE的剩余空间，如果大于 */
            /* 的话就只拷贝LWIP_DEMO_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据 */
            if (q->len > (UDP_NTP_RX_BUFFSIZE - data_len))
                memcpy(g_ntp_demo_recvbuf + data_len, q->payload, (UDP_NTP_RX_BUFFSIZE - data_len)); /* 拷贝数据 */
            else
                memcpy(g_ntp_demo_recvbuf + data_len, q->payload, q->len);

            data_len += q->len;

            if (data_len > UDP_NTP_RX_BUFFSIZE)
                break; /* 超出UDP客户端接收数组,跳出 */
        }
        pbuf_free(p); /* 释放内存 */
    }
    else
    {
        udp_disconnect(upcb);
    }
    udp_ntp_get_time(g_ntp_demo_recvbuf, 40);
    printf("%02d-%02d-%02d %02d:%02d:%02d\r\n",
           g_nowdate.year,
           g_nowdate.month + 1,
           g_nowdate.day,
           g_nowdate.hour + 8,
           g_nowdate.minute,
           g_nowdate.second);
    dns_flag = 0;
}

static void udp_ntp_send_message(struct udp_pcb *u_pcb)
{
    struct pbuf *ptr;
    udp_ntp_message_init();
    ptr = pbuf_alloc(PBUF_TRANSPORT, UDP_NTP_TX_BUFFSIZE, PBUF_POOL); /* 申请内存 */

    if (ptr)
    {
        pbuf_take(ptr, (char *)g_ntp_message_sendbuff, UDP_NTP_TX_BUFFSIZE);
        udp_send(u_pcb, ptr); /* udp发送数据 */
        pbuf_free(ptr);       /* 释放内存 */
    }
}

static void udp_ntp_disconnect(struct udp_pcb *u_pcb)
{
    udp_disconnect(u_pcb);
    udp_remove(u_pcb);
}

void udp_ntp_init(void)
{
    err_t err;
    ip_addr_t remote_addr; /* 远端ip地址 */

    udp_ntp_dns_get_ip();
    //    if (udppcb)
    //    {
    //        IP4_ADDR(&remote_addr, 203,107,6,88);
    //        err = udp_connect(udppcb, &remote_addr, 123);
    //        udp_ntp_send_message(udppcb);
    //    }

    if (udp_ntp_pcb && dns_flag) /* 创建成功 */
    {
        //        printf("Remote IP:%d.%d.%d.%d\r\n", udp_ntp_dns_result[3], udp_ntp_dns_result[2], udp_ntp_dns_result[1], udp_ntp_dns_result[0]); /* 远端IP */
        IP4_ADDR(&remote_addr, udp_ntp_dns_result[3], udp_ntp_dns_result[2], udp_ntp_dns_result[1], udp_ntp_dns_result[0]);
        err = udp_connect(udp_ntp_pcb, &remote_addr, 123); /* UDP客户端连接到指定IP地址和端口号的服务器 */
        if (err == ERR_OK)
        {
            err = udp_bind(udp_ntp_pcb, IP_ADDR_ANY, 123); /* 绑定本地IP地址与端口号 */

            if (err == ERR_OK) /* 绑定完成 */
            {
                // udp_ntp_send_message(udppcb);
                udp_recv(udp_ntp_pcb, udp_ntp_recv_cbk, NULL); /* 注册接收回调函数 */
                // delay_10ms(1);
                udp_ntp_send_message(udp_ntp_pcb);
            }
        }
    }
    else
    {
        udp_ntp_disconnect(udp_ntp_pcb);
    }
}
