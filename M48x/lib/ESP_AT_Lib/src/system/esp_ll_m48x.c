/**
 * \file            esp_ll_m48x.c
 * \brief           Generic M48x driver, included in various M48x driver variants
 */

/*
 * Copyright (c) 2019 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */

/*
 * How it works
 *
 * On first call to \ref esp_ll_init, new thread is created and processed in usart_ll_thread function.
 * USART is configured in RX DMA mode and any incoming bytes are processed inside thread function.
 * DMA and USART implement interrupt handlers to notify main thread about new data ready to send to upper layer.
 *
 *
 * \ref ESP_CFG_INPUT_USE_PROCESS must be enabled in `esp_config.h` to use this driver.
 */

#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"
#include "system/esp_ll.h"

//#include "FreeRTOS.h"
//#include "task.h"

#include "NuMicro.h"
#include "hal/M48x_UART.h"

#ifndef __cplusplus
#ifndef _BOOL
typedef unsigned char bool;
#define False	(0)
#define True	(1)
#endif
#endif 

#if !__DOXYGEN__

#if !ESP_CFG_INPUT_USE_PROCESS
#error "ESP_CFG_INPUT_USE_PROCESS must be enabled in `esp_config.h` to use this driver."
#endif /* ESP_CFG_INPUT_USE_PROCESS */

#if !defined(ESP_USART_DMA_RX_BUFF_SIZE)
#define ESP_UART_DMA_RX_BUFF_SIZE      256	
#endif /* !defined(ESP_USART_DMA_RX_BUFF_SIZE) */

#if !defined(ESP_MEM_SIZE)
#define ESP_MEM_SIZE                    0x1000
#endif /* !defined(ESP_MEM_SIZE) */

typedef struct{
	uint32_t u32DateByte;
	uint8_t au8Buf[ESP_UART_DMA_RX_BUFF_SIZE + 10];
}S_DMA_RXBuf;


static uart_t s_sUARTObj = {
	.uart = UART1,
	.dma_usage = DMA_USAGE_NEVER,
    .dma_chn_id_tx = 0,
	.dma_chn_id_rx = 0,
    .event = 0,
    .hdlr_async = 0,
    .hdlr_dma_tx = NULL,
    .hdlr_dma_rx = NULL,
};

static bool s_bInitialized = False; 

static esp_sys_mbox_t 	s_sUART_ll_rx_mbox;
static esp_sys_mbox_t 	s_sUART_ll_tx_mbox;
static esp_sys_thread_t s_sUART_ll_thread;

#define ESP_UART_DMA_RX_BUFF_BLOCK	8

static S_DMA_RXBuf s_sDMARxBuf[ESP_UART_DMA_RX_BUFF_BLOCK];
static int32_t s_i32DMARxBufIn;
static int32_t s_i32DMARxBufOut;
static int32_t s_i32CurBaudRate;


/**
 * \brief           USART data processing
 */
static void
ESP_UART_ll_thread(
	void *arg
) 
{
	void *pvMsg;
//	printf("[Debug ESP UART: ESP_UART_ll_thread start \n");

	while(1){
		esp_sys_mbox_get(&s_sUART_ll_rx_mbox, &pvMsg, 0); //Wait until "mbox put"

		if((s_i32DMARxBufIn == s_i32DMARxBufOut) && (s_sDMARxBuf[s_i32DMARxBufOut].u32DateByte)){
			esp_input_process(s_sDMARxBuf[s_i32DMARxBufOut].au8Buf, s_sDMARxBuf[s_i32DMARxBufOut].u32DateByte);
			s_sDMARxBuf[s_i32DMARxBufOut].u32DateByte = 0;
			
			if(((s_i32DMARxBufOut + 1) % ESP_UART_DMA_RX_BUFF_BLOCK) == 0)
				s_i32DMARxBufOut = 0;
			else
				s_i32DMARxBufOut ++;
				
			UART_DMA_RX_Start(&s_sUARTObj, s_sDMARxBuf[s_i32DMARxBufIn].au8Buf, ESP_UART_DMA_RX_BUFF_SIZE);
			printf("[Debug ESP UART]: resume rx dma trans \n");
		}

		while(s_i32DMARxBufIn != s_i32DMARxBufOut){
			if(s_sDMARxBuf[s_i32DMARxBufOut].u32DateByte){
				s_sDMARxBuf[s_i32DMARxBufOut].au8Buf[s_sDMARxBuf[s_i32DMARxBufOut].u32DateByte] = 0;
//	printf("[Debug ESP UART: ESP_UART_ll_thread get data %s \n", s_sDMARxBuf[s_i32DMARxBufOut].au8Buf);
				esp_input_process(s_sDMARxBuf[s_i32DMARxBufOut].au8Buf, s_sDMARxBuf[s_i32DMARxBufOut].u32DateByte);
				s_sDMARxBuf[s_i32DMARxBufOut].u32DateByte = 0;

				if(((s_i32DMARxBufOut + 1) % ESP_UART_DMA_RX_BUFF_BLOCK) == 0)
					s_i32DMARxBufOut = 0;
				else
					s_i32DMARxBufOut ++;
			}
			else{
				break;
			}
		}
	}
}


static void
ESP_UART_DMA_TxDoneCB(
	void *pvObj, 
	int i32Event
)
{
//	uart_t *psUartObj = (uart_t *)pvObj;
	
	if(i32Event & DMA_EVENT_TRANSFER_DONE){
		esp_sys_mbox_putnow(&s_sUART_ll_tx_mbox, NULL);
	}
}

static void
ESP_UART_DMA_RxDoneCB(
	void *pvObj, 
	int i32Event
)
{
	uart_t *psUartObj = (uart_t *)pvObj;
	uint32_t u32TransDateByte = dma_untransfer_bytecount(psUartObj->dma_chn_id_rx);

#if 0	
	if(u32TransDateByte == 0){
		printf("ESP_UART_DMA_RxDoneCB \n");
		u32TransDateByte = ESP_UART_DMA_RX_BUFF_SIZE;
	}
	else{
		u32TransDateByte = ESP_UART_DMA_RX_BUFF_SIZE - (u32TransDateByte); 
	}
#else
	if(i32Event & DMA_EVENT_TRANSFER_DONE){
		u32TransDateByte = ESP_UART_DMA_RX_BUFF_SIZE;
	}
	else{
		u32TransDateByte = ESP_UART_DMA_RX_BUFF_SIZE - (u32TransDateByte); 
	}
#endif	
	
	if(i32Event & DMA_EVENT_TIMEOUT){
		//todo restart dma rx
	}

	UART_DMA_RX_Stop(&s_sUARTObj);
	
	if(u32TransDateByte){
		s_sDMARxBuf[s_i32DMARxBufIn].u32DateByte = u32TransDateByte;

		if(((s_i32DMARxBufIn + 1) % ESP_UART_DMA_RX_BUFF_BLOCK) == 0)
			s_i32DMARxBufIn = 0;
		else
			s_i32DMARxBufIn ++;
	}
		
	if(s_sDMARxBuf[s_i32DMARxBufIn].u32DateByte == 0){
		UART_DMA_RX_Start(&s_sUARTObj, s_sDMARxBuf[s_i32DMARxBufIn].au8Buf, ESP_UART_DMA_RX_BUFF_SIZE);
	}
	else{
		printf("[Debug ESP UART]: suspend rx dma trans \n");
	}

	if(u32TransDateByte){
		esp_sys_mbox_putnow(&s_sUART_ll_rx_mbox, NULL);
	}
}


/*
#typedef ESP_UART_TX_PIN  --->	PH9
#typedef ESP_UART_RX_PIN  --->	PH8
#typedef ESP_UART_RST_PIN --->	PH3
#typedef ESP_UART_RTS_PIN --->	PA1
#typedef ESP_UART_CTS_PIN --->	PA0
*/


static int configure_uart(
	uint32_t u32BaudRate
)
{
	UART_InitTypeDef sUARTInit;
	
	s_sUARTObj.uart = (UART_T *)esp_ll_get_uart_obj();
	esp_ll_switch_pin_fun(1);
	esp_ll_hardreset();
	
	//Open UART port
	sUARTInit.u32BaudRate = u32BaudRate;
	sUARTInit.u32DataWidth = UART_WORD_LEN_8;
	sUARTInit.u32Parity = UART_PARITY_NONE;
	sUARTInit.u32StopBits = UART_STOP_BIT_1;
	sUARTInit.eFlowControl = eUART_HWCONTROL_CTS | eUART_HWCONTROL_RTS;
	
	s_i32CurBaudRate = u32BaudRate;

	if(s_bInitialized){
		if(u32BaudRate != s_i32CurBaudRate){
			UART_SetLineConfig(s_sUARTObj.uart, sUARTInit.u32BaudRate, sUARTInit.u32DataWidth, sUARTInit.u32Parity, sUARTInit.u32StopBits);
		}
		return espOK;
	}

	if(UART_Init(&s_sUARTObj, &sUARTInit, 0) != 0){
		printf("Unable open UART port for ESP8266 module \n");
		return espERR;
	}

	if(UART_DMA_TXRX_Enable(&s_sUARTObj, ESP_UART_DMA_TxDoneCB, ESP_UART_DMA_RxDoneCB) != 0){
		printf("Unable enable UART DMA TX/RX for ESP8266 module \n");
		UART_Final(&s_sUARTObj);
		return espERR;	
	}

	if (!esp_sys_mbox_create(&s_sUART_ll_rx_mbox, ESP_UART_DMA_RX_BUFF_BLOCK)) {
		printf("Unable crate UART low level mail box for ESP8266 module\n");
		UART_DMA_TXRX_Disable(&s_sUARTObj);
		UART_Final(&s_sUARTObj);
		return espERR;
	}

	if (!esp_sys_mbox_create(&s_sUART_ll_tx_mbox, 1)) {
		printf("Unable crate UART low level mail box for ESP8266 module\n");
		esp_sys_mbox_delete(&s_sUART_ll_rx_mbox);
		UART_DMA_TXRX_Disable(&s_sUARTObj);
		UART_Final(&s_sUARTObj);
		return espERR;
	}
    
    if (!esp_sys_thread_create(&s_sUART_ll_thread, "esp_uart_ll", ESP_UART_ll_thread, NULL, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO)) {
		printf("Unable crate UART low level thread for ESP8266 module\n");
		esp_sys_mbox_delete(&s_sUART_ll_rx_mbox);
		esp_sys_mbox_delete(&s_sUART_ll_tx_mbox);
		UART_DMA_TXRX_Disable(&s_sUARTObj);
		UART_Final(&s_sUARTObj);
		return espERR;
    }
    
    
    memset(&s_sDMARxBuf, 0 , sizeof(S_DMA_RXBuf) * ESP_UART_DMA_RX_BUFF_BLOCK);
	s_i32DMARxBufIn = 0;
	s_i32DMARxBufOut = 0;

	// start RX dma
	UART_DMA_RX_Start(&s_sUARTObj, s_sDMARxBuf[s_i32DMARxBufIn].au8Buf, ESP_UART_DMA_RX_BUFF_SIZE);

	return espOK;
}
	



/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
send_data(const void* data, size_t len) {
	
	if(UART_DMA_TX_Start(&s_sUARTObj, (uint8_t *)data, len) == 0){
		//Wait trans done
		void *pvMsg;

		esp_sys_mbox_get(&s_sUART_ll_tx_mbox, &pvMsg, 500);	//Wait 500 ms
		UART_DMA_TX_Stop(&s_sUARTObj);
		return len;
	}
	
	//error
	UART_DMA_TX_Stop(&s_sUARTObj);
	return 0;
}


/**
 * \brief           Callback function called from initialization process
 * \note            This function may be called multiple times if AT baudrate is changed from application
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \param[in]       baudrate: Baudrate to use on AT port
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
#if !ESP_CFG_MEM_CUSTOM
    static uint8_t memory[ESP_MEM_SIZE];
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };

    if (!s_bInitialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations */
    }
#endif /* !ESP_CFG_MEM_CUSTOM */
    
    if (!s_bInitialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
#if defined(ESP_RESET_PIN)
//        ll->reset_fn = reset_device;            /* Set callback for hardware reset */
#endif /* defined(ESP_RESET_PIN) */
    }
	
    configure_uart(ll->uart.baudrate);          /* Initialize UART for communication */
    s_bInitialized = True;
    return espOK;
}


/**
 * \brief           Callback function to de-init low-level communication part
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ll_deinit(esp_ll_t* ll) {

	printf("[Debug ESP UART]: esp_ll_deinit \n");

	if(s_bInitialized){	
		// stop RX dma
		UART_DMA_RX_Stop(&s_sUARTObj);

		esp_sys_thread_terminate(&s_sUART_ll_thread);
		esp_sys_mbox_delete(&s_sUART_ll_rx_mbox);
		esp_sys_mbox_delete(&s_sUART_ll_tx_mbox);
		UART_DMA_TXRX_Disable(&s_sUARTObj);
		UART_Final(&s_sUARTObj);
	}
    
    s_bInitialized = False;

	esp_ll_switch_pin_fun(0);

    ESP_UNUSED(ll);
    return espOK;
}


#endif

