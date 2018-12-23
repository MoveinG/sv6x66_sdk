

#include "fsal.h"
#include "osal.h"
#include "hsuart/hal_hsuart.h"
#include "hsuart/drv_hsuart.h"
#include "user_uart.h"
#include "wifi_api.h"
#include "user_transport.h"
//#include "atcmd.h"




AppUartRx_t AppUartRxData1/*,AppUartRxData2*/;
AppUartRx_t *AppUartRx;
OsSemaphore AppUartRxSem = NULL;



void app_uart_int(void);
void app_uart_set_format(uint32_t baudrate,uint8_t databits,uint8_t stopbit,uint8_t paritybit);
void app_uart_send(uint8_t *data,int32_t len);
void app_uart_isr(void);
void app_uart_rx_task(void *pdata);






/*****************************************************************************
*
* app_uart_int
*
* @param null
*        
* @return null
*
* @brief high speed uart int(gpio5--RTS,gpio6--RXD,gpio7--TXD,gpio9--CTS,"custom_io_hal.h")
*
******************************************************************************/
void app_uart_int(void)
{
#if 1
    printf("hsuart int\r\n");
	memset(&AppUartRxData1,0,sizeof(AppUartRx_t));
	AppUartRx = &AppUartRxData1;
    drv_hsuart_init ();
    drv_hsuart_sw_rst ();
    drv_hsuart_set_fifo (HSUART_INT_RX_FIFO_TRIG_LV_16);
    drv_hsuart_set_hardware_flow_control (16, 24);

    drv_hsuart_set_format(9600, HSUART_WLS_8,HSUART_STB_1 , HSUART_PARITY_DISABLE);
    //app_uart_set_format(CIB.uartcfg.baudrate,CIB.uartcfg.databits,CIB.uartcfg.stopbit,CIB.uartcfg.datapaity);

	drv_hsuart_register_isr(HSUART_RX_DATA_READY_IE, app_uart_isr);
	if(OS_SemInit(&AppUartRxSem, 1, 0) == OS_SUCCESS)
	{
       //OS_TaskCreate(app_uart_rx_task, "app uart rx task", 512, NULL, APP_UART_RX_TASK_PRIORITY, NULL);
	}
	else
	{
       printf("app uart rx sem init fail\r\n");
	}
#endif
}

/*****************************************************************************
*
* app_uart_send
*
* @param:*data--uart send data
*        len--data length
* @return:null
*
* @brief:application uart send
*
******************************************************************************/
void app_uart_set_format(uint32_t baudrate,uint8_t databits,uint8_t stopbit,uint8_t paritybit)
{
    HSUART_WLS_E wls;
	HSUART_STB_E stb;
	HSUART_PARITY_E parity;

    switch(databits)
   	{
        case 5:
          wls = HSUART_WLS_5;
		break;

		case 6:
		  wls = HSUART_WLS_6;

		break;

		case 7:
          wls = HSUART_WLS_7;
		break;

		case 8:
		  wls = HSUART_WLS_8;	
		break;
		
        default:
		  wls = HSUART_WLS_8;	
        break;
    }
	switch(stopbit)
	{
        case 1:
          stb = HSUART_STB_1;
		break;

		case 2:
          stb = HSUART_STB_2;
		break;

		default:
		  stb = HSUART_STB_1;	
        break;
	}
	switch(paritybit)
	{
        case 0:
          parity = HSUART_PARITY_DISABLE;
		break;

		case 1:
          parity = HSUART_PARITY_ODD;
		break;

		case 2:
          parity = HSUART_PARITY_EVEN;
		break;

		default:
          parity = HSUART_PARITY_DISABLE;
		break;

	}
    drv_hsuart_set_format((int32_t)baudrate, wls,stb , parity);
}


/*****************************************************************************
*
* app_uart_send
*
* @param:*data--uart send data
*        len--data length
* @return:null
*
* @brief:application uart send
*
******************************************************************************/
void app_uart_send(uint8_t *data,int32_t len)
{
  drv_hsuart_write_fifo(data,len, HSUART_BLOCKING);
}

/*****************************************************************************
*
* app_uart_isr
*
* @param:null
*        
* @return:null
*
* @brief:application uart isr function
*
******************************************************************************/
void app_uart_isr(void)
{
    
    uint32_t len;

    len = drv_hsuart_read_fifo(AppUartRx->buf, APP_UART_BUF_MAX, HSUART_NON_BLOCKING);
	if(len > 0)
	{
	   	AppUartRx->buf[len] = '\0';
		printf("AppUartRx:%s\r\n",AppUartRx->buf);
	   	app_uart_command(AppUartRx->buf,len);
	}
	else
	{
       printf("hsuart isr\r\n");
	}
}

void app_uart_command(char *cmd,char cmdLen)
{
	printf("len:%d\r\n",cmdLen);

	switch (cmd[cmdLen-1])
	{	
		 case '?':
		 	if (get_device_mode() == DUT_AP) {
				deviceStatus.sendToClientEn = 1;
				memcpy(deviceStatus.socketServerRevBuffer ,cmd,cmdLen);
			} else {
		 		printf("uartBuf=%s\r\n",cmd);
				deviceStatus.uartCmdFlag = true;
		 		memcpy(deviceStatus.uartCmdBuffer,cmd,cmdLen);
			}
		 break;

		 case '}':
		 	get_wifi_param(deviceStatus.wifiSsid,deviceStatus.wifiKey);
			set_socket_send_ack(true);
		 break;
		 
		 default:
		 	printf("ddd uartBuf=%s\r\n",cmd);
		 break;
	}
}

void app_server_command(char *cmd)
{
	set_rev_server_data_flag(true);
	switch (*cmd)
	{	
		 case 0x21:
		 break;
		 
		 case 0x22:
		 break;

		 case 0x23:
		 break;

		 case 0x24:
		 break;

		 case 0x25:
		 break;

		 case 0x26:
		 break;

		 case 0x27:
		 break;

		 case 0x5e:
		 break;
		 
		 default:
		 	//set_rev_server_data_flag(false);
		 	printf("server command error!\r\n");
		 break;
	}
}



/*****************************************************************************
*
* app_uart_rx_task
*
* @param:*pdata--null
*        
* @return:null
*
* @brief:appliction uart rx task
*
******************************************************************************/
void app_uart_rx_task(void *pdata)
{
#if 1
   OS_STATUS ret;
   uint32_t last_recv_len = 0;
   bool rx_full_flg = false;
   
   while(1)
   {
      ret = OS_SemWait(AppUartRxSem, APP_UART_RX_TIMEOUT/OS_TICK_RATE_MS);
      if(ret == OS_TIMEOUT)
      {
         if(AppUartRx->recv_len > 0)
         {
            if(AppUartRx->recv_len == last_recv_len)
            {
			   //app_uart_send(AppUartRx->buf,AppUartRx->recv_len);
			   app_uart_command(AppUartRx->buf,AppUartRx->recv_len);
			   AppUartRx->buf[AppUartRx->recv_len] = '\0';
			   printf("AppUartRx:%s\r\n",AppUartRx->buf);
               AppUartRx->recv_len = 0;
            }
			last_recv_len = AppUartRx->recv_len;
		 }
		 rx_full_flg = false;
      }
	  else if(ret = OS_SUCCESS)
	  {
	      //AppUartProcessing(AppUartRx->buf,AppUartRx->recv_len);
	      //app_uart_send(AppUartRx->buf,AppUartRx->recv_len);
		  app_uart_command(AppUartRx->buf,AppUartRx->recv_len);
		  AppUartRx->buf[AppUartRx->recv_len] = '\0';
		  printf("AppUartRx:%s\r\n",AppUartRx->buf);
          //printf("app uart(full):%d\r\n",AppUartRx->recv_len);
          AppUartRx->recv_len = 0;
		  rx_full_flg = true;
	  }
   }
#endif
}











