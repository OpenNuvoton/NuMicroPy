/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Daniel Campora
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"

#if MICROPY_WLAN_ESP8266
#include "esp/esp.h"
#include "esp/esp_dns.h"
#include "system/esp_ll.h"

#include "FreeRTOS.h"
#include "task.h"

#include "NuMicro.h"

#include "modnetwork.h"
#include "lib/netutils/netutils.h"

#define ESP_RESET_PIN PH3
#define ESP_RESET_PIN_PORT PH
#define ESP_RESET_PIN_PIN BIT3

uint8_t esp_ll_hardreset(uint8_t state)
{
	GPIO_SetMode(ESP_RESET_PIN_PORT, ESP_RESET_PIN_PIN, GPIO_MODE_OUTPUT);
	//Hard reset pin, active low
	ESP_RESET_PIN = ~state;
	return 1; //Need to return 1
}

void esp_ll_switch_pin_fun(
	int bUARTMode
)
{
	if(bUARTMode){
		//Setup UART1 multi function pin
		//TX/RX
		SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH8MFP_Msk | SYS_GPH_MFPH_PH9MFP_Msk);
		SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH8MFP_UART1_TXD | SYS_GPH_MFPH_PH9MFP_UART1_RXD);

#if ESP_CFG_UART_FLOW_CONTROL
        //CTS/RTS
        SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk);
        SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB9MFP_UART1_nCTS | SYS_GPB_MFPH_PB8MFP_UART1_nRTS);
#endif

		//Hard reset pin
//		GPIO_SetMode(ESP_RESET_PIN_PORT, ESP_RESET_PIN_PIN, GPIO_MODE_OUTPUT);
	}
	else{
#if ESP_CFG_UART_FLOW_CONTROL
		SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH8MFP_Msk | SYS_GPH_MFPH_PH9MFP_Msk);
        SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk);
#endif
		SYS->GPH_MFPL &= ~(SYS_GPH_MFPL_PH3MFP_Msk);
	}
}

void* esp_ll_get_uart_obj(void)
{
	return (void*)UART1;
}


/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_callback_func(esp_evt_t* evt) {
    switch (esp_evt_get_type(evt)) {
        case ESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            esp_sw_version_t v_min, v_curr;

            esp_get_min_at_fw_version(&v_min);
            esp_get_current_at_fw_version(&v_curr);

//            printf("Current ESP8266 AT version is not supported by library!\r\n");
//            printf("Minimum required AT version is: %d.%d.%d\r\n", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
//            printf("Current AT version is: %d.%d.%d\r\n", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
            break;
        }
        case ESP_EVT_INIT_FINISH: {
//            printf("Library initialized!\r\n");
            break;
        }
        case ESP_EVT_RESET: {
            printf("Device reset sequence finished!\r\n");
            break;
        }
        case ESP_EVT_RESET_DETECTED: {
            printf("Device reset detected!\r\n");
            break;
        }
        case ESP_EVT_WIFI_IP_ACQUIRED: {        /* We have IP address and we are fully ready to work */

            break;
        }

        default: break;
    }
    return espOK;
}

/**
 * \brief           Callback function for access points operation
 */
static espr_t
access_points_cb(esp_evt_t* evt) {
    switch (esp_evt_get_type(evt)) {
        case ESP_EVT_STA_LIST_AP: {
//            printf("Access points listed!\r\n");
            break;
        }
        case ESP_EVT_WIFI_CONNECTED: {
//            printf("Wifi connected!\r\n");
            break;
        }
        case ESP_EVT_WIFI_GOT_IP: {
//            printf("Wifi got IP!\r\n");
            break;
        }
        case ESP_EVT_WIFI_DISCONNECTED: {
//            printf("Wifi disconneted!\r\n");
            break;
        }
        case ESP_EVT_STA_JOIN_AP: {
//            printf("Wifi ioin AP!\r\n");
			break;
        }
        default: break;
    }
    return espOK;
}


/******************************************************************************/
// MicroPython bindings; ESP8266 class

typedef struct _esp8266_if_obj_t {
    mp_obj_base_t base;
    bool bInitialized;
//	struct netif netif;
 //   bool active;
 //   bool dhcpc_eanble;
//	ip4_addr_t ipaddr;
//	ip4_addr_t netmask;
//	ip4_addr_t gw;
//	ip4_addr_t dns_addr;
	
} esp8266_if_obj_t;

STATIC esp8266_if_obj_t esp8266_obj = {
	.base = {(mp_obj_type_t *)&mod_network_nic_type_esp8266},
	.bInitialized = false
};


// \classmethod \constructor(uart, pin_rxs, pin_txs, pin_cts, pin_rts, pin_hwrst)
// Initialise the ESSP8266 using the given uart bus and pins and return a esp8266 object.
//
// Note: pins were originally hard-coded to:
//      PYBv1.0: init(pyb.UART(1), pyb.Pin.board.Y5, pyb.Pin.board.Y4, pyb.Pin.board.Y3, pyb.Pin.board.xx, pyb.Pin.board.xx)
STATIC mp_obj_t esp8266_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	// check arguments
	esp8266_if_obj_t *self = &esp8266_obj;

	mp_arg_check_num(n_args, n_kw, 0, 6, false);

    if (esp_init(esp_callback_func, 1) != espOK) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "failed to init ESP8266 module"));
    }

#if 1
	#define DEF_SPEED_UP	(115200)
	/* Set baudrate to DEF_SPEED_UP and enable flow-control function. */
	if ( esp_set_at_baudrate(DEF_SPEED_UP, NULL, NULL, 1)  != espOK) 
	{
		printf("Cannot set baudrate to %d!\r\n", DEF_SPEED_UP);
	}
#endif

    esp_evt_register(access_points_cb);         /* Register access points */	

    // register with network module
    mod_network_register_nic((mp_obj_t)self);
	esp8266_obj.bInitialized = true;
	return (mp_obj_t)&esp8266_obj;
}



// method connect(ssid, key=None, *, security=WPA2, bssid=None)
STATIC mp_obj_t esp8266_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_ssid, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_key, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get ssid
    size_t ssid_len;
    const char *ssid = mp_obj_str_get_data(args[0].u_obj, &ssid_len);

    // get key and sec
    size_t key_len = 0;
    const char *key = NULL;

    if (args[1].u_obj != mp_const_none) {
        key = mp_obj_str_get_data(args[1].u_obj, &key_len);
    }

    // connect to AP
    if (esp_sta_join(ssid, key, NULL, 1, NULL, NULL, 1) != espOK) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "could not connect to ssid=%s, key=%s\n", ssid, key));
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(esp8266_connect_obj, 1, esp8266_connect);

STATIC mp_obj_t esp8266_disconnect(mp_obj_t self_in) {
    // should we check return value?
    esp_sta_quit(NULL, NULL, 1);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8266_disconnect_obj, esp8266_disconnect);

STATIC mp_obj_t esp8266_isconnected(mp_obj_t self_in) {
    return mp_obj_new_bool(esp_sta_is_joined());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8266_isconnected_obj, esp8266_isconnected);


STATIC mp_obj_t esp8266_ifconfig(mp_obj_t self_in) {
	esp_ip_t ip;
	esp_ip_t gw;
	esp_ip_t nm;	
	uint8_t is_dhcp;
	esp_sta_info_ap_t sSTAInfo;
	esp_mac_t sMAC;
    // render MAC address
    VSTR_FIXED(szMAC_vstr, 18);
	int i32SSIDLen;
	
	memset(&sSTAInfo, 0 ,sizeof(esp_sta_info_ap_t));
	memset(&sMAC, 0, sizeof(esp_mac_t));

	esp_sta_copy_ip(&ip, &gw, &nm, &is_dhcp);
	esp_sta_get_ap_info(&sSTAInfo, NULL, NULL, 1);
	esp_sta_getmac(&sMAC, 0, NULL, NULL, 1);

    vstr_printf(&szMAC_vstr, "%02x:%02x:%02x:%02x:%02x:%02x", sMAC.mac[5], sMAC.mac[4], sMAC.mac[3], sMAC.mac[2], sMAC.mac[1], sMAC.mac[0]);

	i32SSIDLen = strlen(sSTAInfo.ssid);
	
	if(i32SSIDLen > ESP_CFG_MAX_SSID_LENGTH)
		i32SSIDLen = ESP_CFG_MAX_SSID_LENGTH;
    // create and return tuple with ifconfig info
    mp_obj_t tuple[5] = {
        netutils_format_ipv4_addr(ip.ip, NETUTILS_BIG),
        netutils_format_ipv4_addr(nm.ip, NETUTILS_BIG),
        netutils_format_ipv4_addr(gw.ip, NETUTILS_BIG),
        mp_obj_new_str(szMAC_vstr.buf, szMAC_vstr.len),
        mp_obj_new_str((const char*)sSTAInfo.ssid, i32SSIDLen),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(tuple), tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8266_ifconfig_obj, esp8266_ifconfig);


static esp_ap_t s_sAPStatus[20];

STATIC mp_obj_t esp8266_scan(mp_obj_t self_in) {
    STATIC const qstr wlan_scan_info_fields[] = {
        MP_QSTR_ssid, MP_QSTR_sec, MP_QSTR_channel, MP_QSTR_rssi
    };

	espr_t eRes;
	size_t i32APCnt;
	
	memset(s_sAPStatus, 0 , sizeof(s_sAPStatus));
	mp_obj_t sAPList = mp_obj_new_list(0, NULL);
	
	if ((eRes = esp_sta_list_ap(NULL, s_sAPStatus, ESP_ARRAYSIZE(s_sAPStatus), &i32APCnt, NULL, NULL, 1)) == espOK){
		int i;
		int string_len;
		for(i = 0; i < i32APCnt; i ++){
			mp_obj_t tuple[5];
			string_len = strlen(s_sAPStatus[i].ssid);
			if(string_len > (ESP_CFG_MAX_SSID_LENGTH -1))
				string_len = (ESP_CFG_MAX_SSID_LENGTH - 1);
			
			tuple[0] = mp_obj_new_str((const char *)s_sAPStatus[i].ssid, string_len);

			if(s_sAPStatus[i].ecn == ESP_ECN_OPEN){
				string_len = strlen(qstr_str(MP_QSTR_OPEN));
				tuple[1] = mp_obj_new_str(qstr_str(MP_QSTR_OPEN), string_len);
			}
			else if(s_sAPStatus[i].ecn == ESP_ECN_WEP){
				string_len = strlen(qstr_str(MP_QSTR_WEP));
				tuple[1] = mp_obj_new_str(qstr_str(MP_QSTR_WEP), string_len);
			}
			else if(s_sAPStatus[i].ecn == ESP_ECN_WPA_PSK){
				string_len = strlen(qstr_str(MP_QSTR_WPA_PSK));
				tuple[1] = mp_obj_new_str(qstr_str(MP_QSTR_WPA_PSK), string_len);
			}
			else if(s_sAPStatus[i].ecn == ESP_ECN_WPA2_PSK){
				string_len = strlen(qstr_str(MP_QSTR_WPA2_PSK));
				tuple[1] = mp_obj_new_str(qstr_str(MP_QSTR_WPA2_PSK), string_len);
			}
			else if(s_sAPStatus[i].ecn == ESP_ECN_WPA_WPA2_PSK){
				string_len = strlen(qstr_str(MP_QSTR_WPA_WPA2_PSK));
				tuple[1] = mp_obj_new_str(qstr_str(MP_QSTR_WPA_WPA2_PSK), string_len);
			}
			else if(s_sAPStatus[i].ecn == ESP_ECN_WPA2_Enterprise){
				string_len = strlen(qstr_str(MP_QSTR_WPA2_ENTERPRISE));
				tuple[1] = mp_obj_new_str(qstr_str(MP_QSTR_WPA2_ENTERPRISE), string_len);
			}


			tuple[2] = mp_obj_new_int(s_sAPStatus[i].ch);
			tuple[3] = mp_obj_new_int(s_sAPStatus[i].rssi);

			// add the network to the list
			mp_obj_list_append(sAPList, mp_obj_new_attrtuple(wlan_scan_info_fields, 4, tuple));
		}		
	}

	return sAPList;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8266_scan_obj, esp8266_scan);



STATIC const mp_rom_map_elem_t esp8266_locals_dict_table[] = {

    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&esp8266_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&esp8266_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&esp8266_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&esp8266_ifconfig_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&esp8266_scan_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_OPEN), 			MP_ROM_INT(ESP_ECN_OPEN) },
    { MP_ROM_QSTR(MP_QSTR_WEP), 			MP_ROM_INT(ESP_ECN_WEP) },
    { MP_ROM_QSTR(MP_QSTR_WPA_PSK), 		MP_ROM_INT(ESP_ECN_WPA_PSK) },
    { MP_ROM_QSTR(MP_QSTR_WPA2_PSK), 		MP_ROM_INT(ESP_ECN_WPA2_PSK) },
    { MP_ROM_QSTR(MP_QSTR_WPA_WPA2_PSK),	MP_ROM_INT(ESP_ECN_WPA_WPA2_PSK) },
    { MP_ROM_QSTR(MP_QSTR_WPA2_ENTERPRISE),	MP_ROM_INT(ESP_ECN_WPA2_Enterprise) },

#if 0

    { MP_ROM_QSTR(MP_QSTR_patch_version), MP_ROM_PTR(&cc3k_patch_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_patch_program), MP_ROM_PTR(&cc3k_patch_program_obj) },

#endif
};

STATIC MP_DEFINE_CONST_DICT(esp8266_locals_dict, esp8266_locals_dict_table);

///////////////////////////////////////////////////////////////////////////////////////////
STATIC int esp8266_gethostbyname(mp_obj_t nic, const char *name, mp_uint_t len, uint8_t *out_ip) {

	esp_ip_t ip;
	
	
    if (esp_dns_gethostbyname(name, &ip, NULL, NULL, 1) == espOK) {
//        printf("DNS record for example.com: %d.%d.%d.%d\r\n",
//            (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
		memcpy(out_ip, ip.ip, 4);
    } else {
        printf("Error on DNS resolver\r\n");
		return -1;
    }
	
    return 0;
}

STATIC int esp8266_socket_socket(mod_network_socket_obj_t *socket, int *_errno) {
    if (socket->param.domain != MOD_NETWORK_AF_INET) {
        *_errno = MP_EAFNOSUPPORT;
        return -1;
    }

    esp_netconn_type_t type;
    switch (socket->param.type) {
        case MOD_NETWORK_SOCK_STREAM: type = ESP_CONN_TYPE_TCP; break;
        case MOD_NETWORK_SOCK_DGRAM: type = ESP_CONN_TYPE_UDP; break;
        default: *_errno = MP_EINVAL; return -1;
    }

	esp_netconn_p pNetconn;
    pNetconn = esp_netconn_new(type);
    if (pNetconn == NULL) {
        *_errno = -1;
        return -1;
    }

    // store state of this socket
    socket->state = (mp_int_t)pNetconn;
    socket->peer_closed = false;
    socket->pvUnReadBuf = NULL;
	socket->i32ReadBufOffset = 0;

	return 0;
}

STATIC void esp8266_socket_close(mod_network_socket_obj_t *socket) {
	
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;

	if(pNetconn == NULL)
		return;

	esp_netconn_close(pNetconn);
	esp_netconn_delete(pNetconn);
	socket->state = 0;
}

STATIC int esp8266_socket_bind(mod_network_socket_obj_t *socket, byte *ip, mp_uint_t port, int *_errno) {
	espr_t res;
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;

	if(pNetconn == NULL){
        *_errno = -1;		
		return -1;
	}

	res = esp_netconn_bind(pNetconn, port);
	if (res != espOK) {
        *_errno = res;		
		return -2;
	}

    return 0;
}

STATIC int esp8266_socket_listen(mod_network_socket_obj_t *socket, mp_int_t backlog, int *_errno) {

	espr_t res;
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;

	if(pNetconn == NULL){
        *_errno = -1;		
		return -1;
	}

	res = esp_netconn_listen_with_max_conn(pNetconn, backlog);
	if (res != espOK) {
        *_errno = res;		
		return -2;
	}

    return 0;
}

STATIC int esp8266_socket_accept(mod_network_socket_obj_t *socket, mod_network_socket_obj_t *socket2, byte *ip, mp_uint_t *port, int *_errno) {
    // accept incoming connection
	espr_t res;
	esp_netconn_p pServer = (esp_netconn_p)socket->state;
	esp_netconn_p pClient;

	if(pServer == NULL){
        *_errno = -1;		
		return -1;
	}
	
	res = esp_netconn_accept(pServer, &pClient);
	if (res != espOK) {
        *_errno = res;		
		return -2;
	}

    // store state in new socket object
    socket2->state = (mp_int_t)pClient;

#if 0 //Unable get socket information from new connection handle
    // return ip and port
    *port = lwip_ntohs(addr.sin_port);
    int32_t i32IPAddr = addr.sin_addr.s_addr;
    ip[3] = (byte)((i32IPAddr >> 24) & 0xFF);
    ip[2] = (byte)((i32IPAddr >> 16) & 0xFF);
    ip[1] = (byte)((i32IPAddr >> 8) & 0xFF);
    ip[0] = (byte)(i32IPAddr & 0xFF);
#else
	memset(ip, 0x0, 4);
	*port = 0;
#endif
	return 0;
}


STATIC int esp8266_socket_connect(mod_network_socket_obj_t *socket, byte *ip, mp_uint_t port, int *_errno) {

	espr_t res;
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;
	char achHostName[20];

	if(pNetconn == NULL){
        *_errno = -1;		
		return -1;
	}

	memset(achHostName, 0, 20);	
	sprintf(achHostName, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	res = esp_netconn_connect(pNetconn, achHostName, port);

	if (res != espOK) {
        *_errno = res;		
		return -2;
	}

    return 0;
}

STATIC mp_uint_t esp8266_socket_send(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len, int *_errno) {

	espr_t res;
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;

	if(pNetconn == NULL){
        *_errno = -1;		
		return -1;
	}

	res = esp_netconn_write(pNetconn, buf, len);
	if (res != espOK) {
		printf("ESP8266 send failed %d \n", res);
        *_errno = res;		
		return 0;
	}

	esp_netconn_flush(pNetconn);    /* Flush data to output */
		
	return len;
}

STATIC mp_uint_t esp8266_socket_recv(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len, int *_errno) {

	espr_t res;
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;
	esp_pbuf_p pbuf;
	int recv_len;
	int pbuf_len;

	if(pNetconn == NULL){
        *_errno = -1;		
		return -1;
	}

	if(socket->pvUnReadBuf){
		pbuf =	(esp_pbuf_p) socket->pvUnReadBuf;
	}
	else{
		res = esp_netconn_receive(pNetconn, &pbuf);
		if (res == espCLOSED) {     /* Was the connection closed? This can be checked by return status of receive function */
			socket->peer_closed = true;
			return 0;
		}
	}

	pbuf_len = (int)esp_pbuf_length(pbuf, 1);
	
	recv_len = pbuf_len - socket->i32ReadBufOffset;
	if(recv_len > len){
		recv_len = len;
//		printf("Socket buffer is small, unable receive all data\r\n");
	}
	
	recv_len = esp_pbuf_copy(pbuf, buf, recv_len, socket->i32ReadBufOffset);
	socket->i32ReadBufOffset += recv_len;

	if((socket->i32ReadBufOffset) >= pbuf_len){
		esp_pbuf_free(pbuf);
		socket->i32ReadBufOffset = 0;
		socket->pvUnReadBuf = NULL;	
	}
	else{
		socket->pvUnReadBuf = pbuf;	
	}
    return recv_len;
}


STATIC mp_uint_t esp8266_socket_sendto(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len, byte *ip, mp_uint_t port, int *_errno) {

	esp_ip_t sendto_ip;
	espr_t res;
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;

	if(pNetconn == NULL){
        *_errno = -1;		
		return -1;
	}

	sendto_ip.ip[0] = ip[0];
	sendto_ip.ip[1] = ip[1];
	sendto_ip.ip[2] = ip[2];
	sendto_ip.ip[3] = ip[3];

	res = esp_netconn_sendto(pNetconn, &sendto_ip, port, buf, len);
	if (res != espOK) {
		printf("ESP8266 sendto failed %d \n", res);
        *_errno = res;		
		return 0;
	}
	
	return len;
}

STATIC mp_uint_t esp8266_socket_recvfrom(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len, byte *ip, mp_uint_t *port, int *_errno) {
	//esp8266 library not support recvfrom, so call esp8266_socket_recv directly
	return esp8266_socket_recv(socket, buf, len, _errno);
}

STATIC int esp8266_socket_setsockopt(mod_network_socket_obj_t *socket, mp_uint_t level, mp_uint_t opt, const void *optval, mp_uint_t optlen, int *_errno) {

    mp_printf(&mp_plat_print, "Warning: esp8266.setsockopt() option not implemented\n");
	return -1;
}

STATIC int esp8266_socket_settimeout(mod_network_socket_obj_t *socket, mp_uint_t timeout_ms, int *_errno) {
	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;

	if(pNetconn == NULL){
        *_errno = -1;		
		return -1;
	}

	esp_netconn_set_receive_timeout(pNetconn, timeout_ms);
    return 0;
}

STATIC int esp8266_socket_ioctl(mod_network_socket_obj_t *socket, mp_uint_t request, mp_uint_t arg, int *_errno) {

	esp_netconn_p pNetconn = (esp_netconn_p)socket->state;

    if (request == MP_STREAM_CLOSE) {      
        if (pNetconn != NULL) {
			esp_netconn_delete(pNetconn);
			socket->state = 0;
        }
        return 0;
    }

    *_errno = MP_EINVAL;
    return MP_STREAM_ERROR;
}


const mod_network_nic_type_t mod_network_nic_type_esp8266 = {
    .base = {
        { &mp_type_type },
        .name = MP_QSTR_ESP8266,
        .make_new = esp8266_make_new,
        .locals_dict = (mp_obj_dict_t*)&esp8266_locals_dict,
    },
    
    .gethostbyname = esp8266_gethostbyname,
    .socket = esp8266_socket_socket,
    .close = esp8266_socket_close,
    .bind = esp8266_socket_bind,
    .listen = esp8266_socket_listen,
    .accept = esp8266_socket_accept,
    .connect = esp8266_socket_connect,
    .send = esp8266_socket_send,
    .recv = esp8266_socket_recv,
    .sendto = esp8266_socket_sendto,
    .recvfrom = esp8266_socket_recvfrom,
    .setsockopt = esp8266_socket_setsockopt,
    .settimeout = esp8266_socket_settimeout,    
    .ioctl = esp8266_socket_ioctl,
};


#endif

