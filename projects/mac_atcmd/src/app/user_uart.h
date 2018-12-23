#ifndef USER_UART_H
#define USER_UART_H




#define APP_UART_BUF_MAX                1000
#define APP_UART_RX_TIMEOUT             30      //Unit ms
#define APP_UART_RX_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )


#define CMD_CHAR                        "+++"
#define CMD_CHAR_NUMS                   3




typedef struct 
{
   bool use_flg;
   uint8_t buf[APP_UART_BUF_MAX];
   uint32_t recv_len;
}AppUartRx_t;
	















extern void app_uart_int(void);
extern void app_uart_send(uint8_t *data,int32_t len);
void app_uart_command(char *cmd,char cmdLen);


#endif /* end of include guard: APP_UART_H */

