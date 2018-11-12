#ifndef USER_UART_H
#define USER_UART_H


#include <stdint.h>
#include <stdio.h>


#if DEBUG == 1
#define DBG_LOG(format, ...)    printf("> "format"\r\n", ##__VA_ARGS__)
#define DBG_SEND(data, len)     user_uartSendData(data, len)

/*使能微信蓝牙的LOG输出*/
#define CATCH_LOG

#else
#define DBG_LOG(format, ...)
#define DBG_SEND(data, len)
#endif

#define UART_TX_BUF_SIZE                512        /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                512        /**< UART RX buffer size. */

void user_uartSendData(uint8_t *data, uint16_t len);
void user_uart_init(void);

#endif
