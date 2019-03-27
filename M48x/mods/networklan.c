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


#if MICROPY_NVT_LWIP

#include "modnetwork.h"

#include "hal/M48x_ETH.h"
#include "netif/ethernetif.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"


#include "lib/netutils/netutils.h"

typedef struct _lan_if_obj_t {
    mp_obj_base_t base;
    bool initialized;
	struct netif netif;
    bool active;
    bool dhcpc_eanble;
	ip4_addr_t ipaddr;
	ip4_addr_t netmask;
	ip4_addr_t gw;
	ip4_addr_t dns_addr;
	
} lan_if_obj_t;

const unsigned char my_mac_addr[6] = {0x00, 0x00, 0x00, 0x55, 0x66, 0x77};
//const mod_network_nic_type_t mod_network_nic_type_lan;
extern const mp_obj_type_t mod_network_nic_type_lan;

STATIC lan_if_obj_t lan_obj = {
	.base = {(mp_obj_t)&mod_network_nic_type_lan},
	.initialized = false,
	.active = false,
	.dhcpc_eanble = false,
};

STATIC void switch_pinfun() {
    // Configure RMII pins
    SYS->GPA_MFPL |= SYS_GPA_MFPL_PA6MFP_EMAC_RMII_RXERR | SYS_GPA_MFPL_PA7MFP_EMAC_RMII_CRSDV;
    SYS->GPC_MFPL |= SYS_GPC_MFPL_PC6MFP_EMAC_RMII_RXD1 | SYS_GPC_MFPL_PC7MFP_EMAC_RMII_RXD0;
    SYS->GPC_MFPH |= SYS_GPC_MFPH_PC8MFP_EMAC_RMII_REFCLK;
    SYS->GPE_MFPH |= SYS_GPE_MFPH_PE8MFP_EMAC_RMII_MDC |
                     SYS_GPE_MFPH_PE9MFP_EMAC_RMII_MDIO |
                     SYS_GPE_MFPH_PE10MFP_EMAC_RMII_TXD0 |
                     SYS_GPE_MFPH_PE11MFP_EMAC_RMII_TXD1 |
                     SYS_GPE_MFPH_PE12MFP_EMAC_RMII_TXEN;

    // Enable high slew rate on all RMII TX output pins
    PE->SLEWCTL = (GPIO_SLEWCTL_HIGH << GPIO_SLEWCTL_HSREN10_Pos) |
                  (GPIO_SLEWCTL_HIGH << GPIO_SLEWCTL_HSREN11_Pos) |
                  (GPIO_SLEWCTL_HIGH << GPIO_SLEWCTL_HSREN12_Pos);
}



STATIC mp_obj_t lan_active(size_t n_args, const mp_obj_t *args) {
    lan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args > 1) {
        if (mp_obj_is_true(args[1])) {
			netif_set_up(&self->netif);
            self->active = true;
        } else {
			if(self->dhcpc_eanble){
				dhcp_release(&self->netif);
				dhcp_stop(&self->netif);
				self->dhcpc_eanble = false;
			}
			
			netif_set_down(&self->netif);
            self->active = false;
        }
    }
    return mp_obj_new_bool(self->active);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lan_active_obj, 1, 2, lan_active);

STATIC mp_obj_t lan_if_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	lan_if_obj_t *self = &lan_obj;

    if (self->initialized) {
        return (mp_obj_t)self;
    }


	switch_pinfun();
	ETH_Init0();
	
	//default ip address
    IP4_ADDR(&self->gw, 192,168,1,1);
    IP4_ADDR(&self->ipaddr, 192,168,1,2);
    IP4_ADDR(&self->netmask, 255,255,255,0);
    IP4_ADDR(&self->dns_addr, 168,95,1,1);

    tcpip_init(NULL, NULL);
    netif_add(&self->netif, &self->ipaddr, &self->netmask, &self->gw, NULL, ethernetif_init, tcpip_input);
    netif_set_default(&self->netif);
	dns_setserver(0, &self->dns_addr);
	
	self->initialized = true;
	return (mp_obj_t)self;
}

STATIC int32_t WaitDHCPResponse(
	struct netif *psNetif
)
{
	int i;
	
	ip_addr_t ip4addr = IPADDR4_INIT(IPADDR_ANY);

	struct dhcp *pDHCPC = netif_dhcp_data(psNetif);
	
	if(pDHCPC == NULL)
		return 1;
		
	for(i = 0; i < 100; i ++){		//wait 5 sec
		ip_addr_copy(ip4addr, *netif_ip_addr4(psNetif));
		if(!ip_addr_isany(&ip4addr)){ 
			return 0;
		}
		mp_hal_delay_ms(100);		
	}

	return 1;
}

STATIC mp_obj_t lan_ifconfig(size_t n_args, const mp_obj_t *args) {
	lan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	if (n_args == 1) {
		// get
		ip_addr_t *p_dns_addr;
		p_dns_addr = dns_getserver(0);

		if(self->active)
			ip_addr_copy(self->dns_addr, *p_dns_addr);

		mp_obj_t tuple[4] = {
			netutils_format_ipv4_addr((uint8_t*)&self->ipaddr, NETUTILS_BIG),
			netutils_format_ipv4_addr((uint8_t*)&self->netmask, NETUTILS_BIG),
			netutils_format_ipv4_addr((uint8_t*)&self->gw, NETUTILS_BIG),
			netutils_format_ipv4_addr((uint8_t*)p_dns_addr, NETUTILS_BIG),
		};
        return mp_obj_new_tuple(4, tuple);
	}
	else{
		//set
		bool bDHCP = false;
		bool bStaticIP = true;

		if (MP_OBJ_IS_TYPE(args[1], &mp_type_tuple)) {
			//static ip address
            mp_obj_t *items;
            mp_obj_get_array_fixed_n(args[1], 4, &items);

            netutils_parse_ipv4_addr(items[0], (uint8_t *)&self->ipaddr, NETUTILS_BIG);
            netutils_parse_ipv4_addr(items[1], (uint8_t *)&self->netmask, NETUTILS_BIG);
            netutils_parse_ipv4_addr(items[2], (uint8_t *)&self->gw, NETUTILS_BIG);
            netutils_parse_ipv4_addr(items[3], (uint8_t *)&self->dns_addr, NETUTILS_BIG);

		}
		else{
			//dhcp  
           const char *mode = mp_obj_str_get_str(args[1]);
            if (strcmp("dhcp", mode)) {
                mp_raise_ValueError("invalid argument");
            }
            
            bDHCP = true;
			bStaticIP = false;
		}
		
		if(self->active){
			//dwon interface
			netif_set_down(&self->netif);
		}
		
		if(bDHCP){
			//up interface
			ip_addr_t ip4addr = IPADDR4_INIT(IPADDR_ANY);
			netif_set_addr(&self->netif, ip_2_ip4(&ip4addr), &self->netmask, &self->gw);
			netif_set_up(&self->netif);

			//start dhcpc
			if(!self->dhcpc_eanble){
				if(dhcp_start(&self->netif) != ERR_OK)
					goto Do_StaticIP;
				self->dhcpc_eanble = true;
			}
			//wait dhcp result
			if(WaitDHCPResponse(&self->netif) == 0){
				//DHCP OK! update ip address
				ip_addr_copy(self->ipaddr, *netif_ip_addr4(&self->netif));
				ip_addr_copy(self->netmask, *netif_ip_netmask4(&self->netif));
				ip_addr_copy(self->gw, *netif_ip_gw4(&self->netif));
				ip_addr_copy(self->dns_addr, *dns_getserver(0));
				return mp_const_true;
			}
			else{
				printf("DHCP get IP failed!\n");
				return mp_const_false;
//				bStaticIP = true;
			}
		}

Do_StaticIP:				
		if(bStaticIP){
			// disable dhcpc
			if(self->dhcpc_eanble){
				dhcp_release(&self->netif);
				dhcp_stop(&self->netif);
				self->dhcpc_eanble = false;
			}
			//set ip address
			netif_set_addr(&self->netif, &self->ipaddr, &self->netmask, &self->gw);

			//up interface
			netif_set_up(&self->netif);
		}

		//start dns service
		dns_setserver(0, &self->dns_addr);
		self->active = true;
	}
	
	return mp_const_true;

}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lan_ifconfig_obj, 1, 2, lan_ifconfig);

STATIC mp_obj_t lan_isconnected(mp_obj_t self_in) {
    lan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->active ? mp_const_true : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(lan_isconnected_obj, lan_isconnected);

STATIC mp_obj_t lan_status(mp_obj_t self_in) {
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(lan_status_obj, lan_status);

STATIC const mp_rom_map_elem_t lan_if_locals_dict_table2[] = {
    { MP_ROM_QSTR(MP_QSTR_active), MP_ROM_PTR(&lan_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&lan_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&lan_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&lan_ifconfig_obj) },
};

STATIC MP_DEFINE_CONST_DICT(lan_if_locals_dict, lan_if_locals_dict_table2);

const mp_obj_type_t mod_network_nic_type_lan = {
        { &mp_type_type },
        .name = MP_QSTR_LAN,
        .make_new = lan_if_make_new,
        .locals_dict = (mp_obj_dict_t *)&lan_if_locals_dict,
};

#endif
