/**
 * \file            esp_ll.h
 * \brief           Low-level communication implementation
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
#ifndef ESP_HDR_LL_H
#define ESP_HDR_LL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "esp/esp.h"

/**
 * \ingroup         ESP_PORT
 * \defgroup        ESP_LL Low-level functions
 * \brief           Low-level communication functions
 * \{
 */

espr_t      esp_ll_init(esp_ll_t* ll);
espr_t      esp_ll_deinit(esp_ll_t* ll);

uint8_t 	esp_ll_hardreset(uint8_t state);			//must implement by application for M48x
void		esp_ll_switch_pin_fun(int bUARTMode);		//must implement by application for M48x. bUARTMode=1:Swith pin to UART pin. bUARTMode=1:Swith pin to GPIO pin 
void*		esp_ll_get_uart_obj(void);					//must implement by application for M48x. Return UART object. Ex: UART1, UART2, ....

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ESP_HDR_LL_H */
