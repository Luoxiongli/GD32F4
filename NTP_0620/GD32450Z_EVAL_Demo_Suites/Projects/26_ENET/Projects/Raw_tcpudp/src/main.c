/*!
    \file    main.c
    \brief   enet demo

    \version 2024-01-17, V2.6.4, demo for GD32F4xx
*/

#include "gd32f4xx.h"
#include "netconf.h"
#include "main.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
#include "gd32f450z_eval.h"
#include "hello_gigadevice.h"
#include "udp_echo.h"
#include "tcp_client.h"

#include "lwip/init.h"

#include "ntp_dns.h"
#include "udp_ntp.h"

#define SYSTEMTICK_PERIOD_MS 10

__IO uint32_t g_localtime = 0; /* for creating a time reference incremented by 10ms */
uint32_t g_timedelay;

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    gd_eval_com_init(EVAL_COM0);
    gd_eval_key_init(KEY_TAMPER, KEY_MODE_EXTI);
    /* setup ethernet system(GPIOs, clocks, MAC, DMA, systick) */
    enet_system_setup();

    /* initilaize the LwIP function */
    lwip_init();
    /* initilaize the LwIP stack */
    lwip_stack_init();

    while (1)
    {
				udp_ntp_init();
#ifndef USE_ENET_INTERRUPT
        /* check if any packet received */
        if (enet_rxframe_size_get())
        {
            /* process received ethernet packet */
            lwip_pkt_handle();
        }
#endif /* USE_ENET_INTERRUPT */

        /* handle periodic timers for LwIP */
#ifdef TIMEOUT_CHECK_USE_LWIP
        sys_check_timeouts();

#ifdef USE_DHCP
        lwip_dhcp_process_handle();
#endif /* USE_DHCP */

#else
        lwip_periodic_handle(g_localtime);
#endif /* TIMEOUT_CHECK_USE_LWIP */
    }
}

/*!
    \brief      after the netif is fully configured, it will be called to initialize the function of telnet, client and udp
    \param[in]  netif: the struct used for lwIP network interface
    \param[out] none
    \retval     none
*/
void lwip_netif_status_callback(struct netif *netif)
{
    if (((netif->flags & NETIF_FLAG_UP) != 0) && (0 != netif->ip_addr.addr))
    {
        // udp_ntp_init();
        // /* initilaize the tcp server: telnet 8000 */
        // hello_gigadevice_init();
        // /* initilaize the tcp client: echo 10260 */
        // tcp_client_init();
        // /* initilaize the udp: echo 1025 */
        // udp_echo_init();
    }
}
/*!
    \brief      insert a delay time
    \param[in]  ncount: number of 10ms periods to wait for
    \param[out] none
    \retval     none
*/
void delay_10ms(uint32_t ncount)
{
    /* capture the current local time */
    g_timedelay = g_localtime + ncount;

    /* wait until the desired delay finish */
    while (g_timedelay > g_localtime)
    {
    }
}

/*!
    \brief      updates the system local time
    \param[in]  none
    \param[out] none
    \retval     none
*/
void time_update(void)
{
    g_localtime += SYSTEMTICK_PERIOD_MS;
}

void _sys_exit(int x)
{
    x = x;
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(EVAL_COM0, (uint8_t)ch);
    while (RESET == usart_flag_get(EVAL_COM0, USART_FLAG_TBE))
        ;
    return ch;
}
