#ifndef UDP_NTP_H_
#define UDP_NTP_H_

#include "lwip/udp.h"
#include "lwip/dns.h"

#include <string.h>
#include <stdio.h>
#include "gd32f4xx.h"
#include "main.h"

#define DNS_TEST 0

#define ALIYUN_NTP_HOSTNAME "ntp1.aliyun.com"
#define IP_LEN 4
#define UDP_NTP_PORT 123
#define UDP_NTP_TX_BUFFSIZE 48
#define UDP_NTP_RX_BUFFSIZE 2000
#define TIMESTAMP_DELTA 2208988800ULL

typedef struct _NTPformat
{
    char version;               /* 版本号 */
    char leap;                  /* 时钟同步 */
    char mode;                  /* 模式 */
    char stratum;               /* 系统时钟的层数 */
    char poll;                  /* 更新间隔 */
    signed char precision;      /* 精密度 */
    unsigned int rootdelay;     /* 本地到主参考时钟源的往返时间 */
    unsigned int rootdisp;      /* 统时钟相对于主参考时钟的最大误差 */
    char refid;                 /* 参考识别码 */
    unsigned long long reftime; /* 参考时间 */
    unsigned long long org;     /* 开始的时间戳 */
    unsigned long long rec;     /* 收到的时间戳 */
    unsigned long long xmt;     /* 传输时间戳 */
} NTPformat;

typedef struct _DateTime
{
    int year;   /* 年 */
    int month;  /* 月 */
    int day;    /* 天 */
    int hour;   /* 时 */
    int minute; /* 分 */
    int second; /* 秒 */
} DateTime;

#define SECS_PERDAY 86400UL /* 60*60*24 */
#define UTC_ADJ_HRS 8       /* GMT+8（东八区北京） */
#define EPOCH 1900          /* NTP 起始年份 */

void udp_ntp_dns_get_ip(void);
void udp_ntp_init(void);

#endif
