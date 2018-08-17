/***********************************************************
*  File: tuya_uart.c
*  Author: nzy
*  Date: 20171106
***********************************************************/
#define __TUYA_UART_GLOBALS
#include <string.h>
#include "tuya_uart.h"
//#include "objects.h"
#include "hsuart/drv_hsuart.h"
#include "uni_log.h"
#include "mem_pool.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
//#define UART_TX PA_23 //UART0 TX
//#define UART_RX PA_18 //UART0 RX 

typedef struct {
    //serial_t sobj;
    UINT_T buf_len;
    BYTE_T *buf;
    USHORT_T in;
    USHORT_T out;
}TUYA_UART_S;

/***********************************************************
*************************variable define********************
***********************************************************/
STATIC TUYA_UART_S ty_uart;

/***********************************************************
*************************function define********************
***********************************************************/
STATIC VOID hsuart_isr(VOID);

/***********************************************************
*  Function: ty_uart_init 
*  Input: port badu bits parity stop
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_init(IN CONST TY_UART_PORT_E port,IN CONST TY_UART_BAUD_E badu,\
                               IN CONST TY_DATA_BIT_E bits,IN CONST TY_PARITY_E parity,\
                               IN CONST TY_STOPBITS_E stop,IN CONST UINT_T bufsz)
{
    printf("%s port=%d, badu=%d, bits=%d, parity=%d, stop=%d, bufsz=%d\n", __func__, port, badu, bits, parity, stop, bufsz);

    if((port != TY_UART0) || (bufsz == 0))
        return OPRT_INVALID_PARM;
    
	if((stop == TYS_STOPBIT1_5) && (bits != TYWL_5B))
        return OPRT_INVALID_PARM;
	
    if(ty_uart.buf == NULL) {
        memset(&ty_uart, 0, sizeof(TUYA_UART_S));
        ty_uart.buf = Malloc(bufsz);
        if(ty_uart.buf == NULL) {
            return OPRT_MALLOC_FAILED;
        }
        ty_uart.buf_len = bufsz;
        PR_DEBUG("uart buf size : %d",bufsz);
    }else {
        return OPRT_COM_ERROR;
    }
    
    INT_T StopBits=0;
    if(stop == TYS_STOPBIT1) {
        StopBits = 0;
    }else if(stop == TYS_STOPBIT2) {
        StopBits = 1;
	}

	INT_T Parity=parity;
    if(Parity == TYP_EVEN) Parity = 3;
	
    drv_hsuart_init ();
    drv_hsuart_sw_rst ();
    drv_hsuart_set_fifo (HSUART_INT_RX_FIFO_TRIG_LV_16);
    drv_hsuart_set_hardware_flow_control (16, 24);

    INT_T retval = drv_hsuart_set_format(badu, bits, StopBits, Parity);
    if(retval != 0)
    {
        Free(ty_uart.buf);
        ty_uart.buf = NULL;
        printf("%s retval=%d\n", retval);
        return OPRT_COM_ERROR;
    }
    //drv_hsuart_register_isr (HSUART_RX_DATA_READY_IE, hsuart_isr);
    
    return OPRT_OK;
}

/***********************************************************
*  Function: ty_uart_free 
*  Input:free uart
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_free(IN CONST TY_UART_PORT_E port)
{
    printf("%s port=%d\n", __func__, port);

    if(port != TY_UART0)
       return OPRT_INVALID_PARM;

    if(ty_uart.buf != NULL) {
        Free(ty_uart.buf);
        ty_uart.buf = NULL;
    }
    ty_uart.buf_len = 0;

    drv_hsuart_deinit();
    return OPRT_OK;
}

/***********************************************************
*  Function: ty_uart_send_data 
*  Input: port data len
*  Output: none
*  Return: none
***********************************************************/
VOID ty_uart_send_data(IN CONST TY_UART_PORT_E port,IN CONST BYTE_T *data,IN CONST UINT_T len)
{
    printf("%s port=%d, data=0x%x, len=%d\n", __func__, port, data, len);

    if(port != TY_UART0 || NULL == data || 0 == len)
        return;

	drv_hsuart_write_fifo(data, len, HSUART_BLOCKING);
}

#if 0
STATIC UINT_T __ty_uart_read_data_size(IN CONST TY_UART_PORT_E port)
{
    UINT_T remain_buf_size = 0;

    if(port != TY_UART0)
		return 0;

    if(ty_uart.in >= ty_uart.out) {
        remain_buf_size = ty_uart.in-ty_uart.out;
    }else {
        remain_buf_size = ty_uart.in + ty_uart.buf_len - ty_uart.out;
    }

    return remain_buf_size;
}


STATIC VOID hsuart_isr(VOID)
{
    //serial_t *sobj = (void*)id;
    INT_T rc = 0;
    
    INT_T i = 0;
    for(i = 0;i < TY_UART_NUM;i++) {
        /*if(&ty_uart[i].sobj == sobj) {
            break;
        }*/
    }
    
    if(0) {//event == RxIrq) {
        //rc = serial_getc(sobj);
        //PR_NOTICE("rc = %d", rc);
        if(/*__ty_uart_read_data_size*/(i) < ty_uart.buf_len - 1) {
            ty_uart.buf[ty_uart.in++] = rc;
            if(ty_uart.in >= ty_uart.buf_len){
                ty_uart.in = 0;
            }
        }else {
            //PR_ERR("uart fifo is overflow! buf_zize:%d  data_get:%d",ty_uart.buf_len,__ty_uart_read_data_size(i));
        }
    }
}
#endif

/***********************************************************
*  Function: ty_uart_send_data 
*  Input: len->data buf len
*  Output: buf->read data buf
*  Return: actual read data size
***********************************************************/
UINT_T ty_uart_read_data(IN CONST TY_UART_PORT_E port,OUT BYTE_T *buf,IN CONST UINT_T len)
{
    printf("%s port=%d, buf=0x%x, len=%d\n", __func__, port, buf, len);

     if(port != TY_UART0 || NULL == buf || 0 == len)
        return 0;

    /*UINT_T actual_size = 0;
    UINT_T cur_num = __ty_uart_read_data_size(port);
    if(cur_num > ty_uart.buf_len - 1) {
        PR_DEBUG("uart fifo is full! buf_zize:%d  len:%d",cur_num,len);
    }

    if(len > cur_num) {
        actual_size = cur_num;
    }else {
        actual_size = len;
    }
    //PR_NOTICE("uart_num = %d", cur_num);
    UINT_T i = 0;
    for(i = 0;i < actual_size;i++) {
        *buf++ = ty_uart.buf[ty_uart.out++];
        if(ty_uart.out >= ty_uart.buf_len) {
            ty_uart.out = 0;
        }
    }*/
	drv_hsuart_read_fifo(buf, len, HSUART_BLOCKING);

    return len;
}



