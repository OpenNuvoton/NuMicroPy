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

#include "hal/N9H26_ETH.h"
#include "netif/ethernetif.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/netdb.h"
#include "lwip/ip4.h"
#include "lwip/igmp.h"

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
extern const mod_network_nic_type_t mod_network_nic_type_lan;

STATIC lan_if_obj_t lan_obj = {
	.base = {(mp_obj_type_t *)&mod_network_nic_type_lan},
	.initialized = false,
	.active = false,
	.dhcpc_eanble = false,
};

STATIC void switch_pinfun() {
    // Configure RMII pins
	outp32(REG_GPCFUN1, 0x44444444);
	outp32(REG_GPEFUN0, inp32(REG_GPEFUN0) &~(0xFF));
	outp32(REG_GPEFUN0, inp32(REG_GPEFUN0) | 0x44);
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
	
    // register with network module
    mod_network_register_nic((mp_obj_t)self);
	
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
		p_dns_addr = (ip_addr_t *)dns_getserver(0);

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

////////////////////////////////////////////////////////////////////////////
//		socket interface
////////////////////////////////////////////////////////////////////////////

#define SOCKET_POLL_US (100000)

static void _socket_settimeout(mod_network_socket_obj_t *sock, uint64_t timeout_ms) {
    // Rather than waiting for the entire timeout specified, we wait sock->retries times
    // for SOCKET_POLL_US each, checking for a MicroPython interrupt between timeouts.
    // with SOCKET_POLL_MS == 100ms, sock->retries allows for timeouts up to 13 years.
    // if timeout_ms == UINT64_MAX, wait forever.
    sock->retries = (timeout_ms == UINT64_MAX) ? UINT_MAX : timeout_ms * 1000 / SOCKET_POLL_US;

    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = timeout_ms ? SOCKET_POLL_US : 0
    };
    lwip_setsockopt(sock->state, SOL_SOCKET, SO_SNDTIMEO, (const void *)&timeout, sizeof(timeout));
    lwip_setsockopt(sock->state, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout));
    lwip_fcntl(sock->state, F_SETFL, timeout_ms ? 0 : O_NONBLOCK);
}

STATIC int lan_gethostbyname(mp_obj_t nic, const char *name, mp_uint_t len, uint8_t *out_ip) {
    struct hostent *hent;
    if((hent = lwip_gethostbyname(name)) == NULL)
    {
        printf("ERROR: gethostbyname error for hostname: %s\n", name);
        return -1;
    }	

	memcpy(out_ip, hent->h_addr_list[0], 4);
    return 0;
}

STATIC int lan_socket_socket(mod_network_socket_obj_t *socket, int *_errno) {
    if (socket->param.domain != MOD_NETWORK_AF_INET) {
        *_errno = MP_EAFNOSUPPORT;
        return -1;
    }

    mp_uint_t type;
    switch (socket->param.type) {
        case MOD_NETWORK_SOCK_STREAM: type = SOCK_STREAM; break;
        case MOD_NETWORK_SOCK_DGRAM: type = SOCK_DGRAM; break;
        case MOD_NETWORK_SOCK_RAW: type = SOCK_RAW; break;
        default: *_errno = MP_EINVAL; return -1;
    }

	int fd;
    fd = lwip_socket(AF_INET, type, 0);
    if (fd < 0) {
        *_errno = fd;
        return -1;
    }

    // store state of this socket
    socket->state = fd;
    socket->peer_closed = false;
    
    _socket_settimeout(socket, UINT64_MAX);
	return 0;
}

STATIC void lan_socket_close(mod_network_socket_obj_t *socket) {
	lwip_close(socket->state);
	socket->state = -1;
}

STATIC int lan_socket_bind(mod_network_socket_obj_t *socket, byte *ip, mp_uint_t port, int *_errno) {
	struct sockaddr_in bind_addr;
	int ret;

	//TODO: check byte order for sockadd_in
	memset(&bind_addr, 0x0 , sizeof(bind_addr));

	bind_addr.sin_len = sizeof(bind_addr);
	bind_addr.sin_family = AF_INET;
	memcpy(&bind_addr.sin_addr.s_addr, ip, 4);
	bind_addr.sin_port = lwip_htons(port);
	
	ret = lwip_bind(socket->state, (struct sockaddr*)&bind_addr, sizeof(bind_addr));

	if(ret != 0){
        *_errno = ret;
        return -1;

	}
    return 0;
}

STATIC int lan_socket_connect(mod_network_socket_obj_t *socket, byte *ip, mp_uint_t port, int *_errno) {

	struct sockaddr_in conn_addr;
	int ret;

	//TODO: check byte order for sockadd_in
	memset(&conn_addr, 0x0 , sizeof(conn_addr));

	conn_addr.sin_len = sizeof(conn_addr);
	conn_addr.sin_family = AF_INET;
	memcpy(&conn_addr.sin_addr.s_addr, ip, 4);
	conn_addr.sin_port = lwip_htons(port);
//	printf("%x and %x\n", conn_addr.sin_addr.s_addr, conn_addr.sin_port);

//	int i;
//	for(i=0; i < 4; i++){
//		printf("ip[%d]:%x \n", i , ip[i]);
//	}

    ret = lwip_connect(socket->state, (struct sockaddr*)&conn_addr, sizeof(conn_addr));
    if (ret != 0) {
        *_errno = ret;
        return -1;
    }
    return 0;
}

STATIC int lan_socket_listen(mod_network_socket_obj_t *socket, mp_int_t backlog, int *_errno) {

    int ret = lwip_listen(socket->state, backlog);
    if (ret < 0){
        *_errno = ret;
        return -1;
	}
    return 0;
}

STATIC int lan_socket_accept(mod_network_socket_obj_t *socket, mod_network_socket_obj_t *socket2, byte *ip, mp_uint_t *port, int *_errno) {
    // accept incoming connection
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int new_fd = -1;
    for (int i=0; i <= socket->retries; i++) {
        new_fd = lwip_accept(socket->state, (struct sockaddr*)&addr, &addr_len);
        if (new_fd >= 0) break;
        if (errno != EAGAIN){
            *_errno = -new_fd;
			return -1;
		}
    }
    
    if (new_fd < 0){
		*_errno = MP_ETIMEDOUT;
		return -2;
	}

    // store state in new socket object
    socket2->state = new_fd;
    _socket_settimeout(socket2, UINT64_MAX);

    // return ip and port
    *port = lwip_ntohs(addr.sin_port);
    int32_t i32IPAddr = addr.sin_addr.s_addr;
    ip[3] = (byte)((i32IPAddr >> 24) & 0xFF);
    ip[2] = (byte)((i32IPAddr >> 16) & 0xFF);
    ip[1] = (byte)((i32IPAddr >> 8) & 0xFF);
    ip[0] = (byte)(i32IPAddr & 0xFF);

	return 0;
}

STATIC mp_uint_t lan_socket_send(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len, int *_errno) {

    int sentlen = 0;
    for (int i = 0; i <= socket->retries && sentlen < len; i++) {
        int r = lwip_write(socket->state, buf+sentlen, len-sentlen);
        if (r < 0 && errno != EWOULDBLOCK){
			*_errno = errno;
			return -1;			
		}
		
        if (r > 0) sentlen += r;
    }

    if (sentlen == 0){
		*_errno = MP_ETIMEDOUT;
		return -1;
	
	}
    return sentlen;
}

// XXX this can end up waiting a very long time if the content is dribbled in one character
// at a time, as the timeout resets each time a recvfrom succeeds ... this is probably not
// good behaviour.
STATIC mp_uint_t _socket_read_data(mod_network_socket_obj_t *socket, void *buf, size_t size,
    struct sockaddr *from, socklen_t *from_len, int *errcode) {

    // If the peer closed the connection then the lwIP socket API will only return "0" once
    // from lwip_recvfrom_r and then block on subsequent calls.  To emulate POSIX behaviour,
    // which continues to return "0" for each call on a closed socket, we set a flag when
    // the peer closed the socket.
    if (socket->peer_closed) {
        return 0;
    }

    // XXX Would be nicer to use RTC to handle timeouts
    for (int i = 0; i <= socket->retries; ++i) {
        int r = lwip_recvfrom(socket->state, buf, size, 0, from, from_len);
        if (r == 0) {
            socket->peer_closed = true;
        }
        if (r >= 0) {
            return r;
        }
        if (errno != EWOULDBLOCK) {
            *errcode = errno;
            return -1;
        }
    }

    *errcode = socket->retries == 0 ? MP_EWOULDBLOCK : MP_ETIMEDOUT;
    return -1;
}


STATIC mp_uint_t lan_socket_recv(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len, int *_errno) {
	mp_uint_t ret = _socket_read_data(socket, buf, len, NULL, NULL, _errno);
    return ret;
}

STATIC mp_uint_t lan_socket_sendto(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len, byte *ip, mp_uint_t port, int *_errno) {

    // create the destination address
    struct sockaddr_in to;
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
	memcpy(&to.sin_addr.s_addr, ip, 4);
	to.sin_port = lwip_htons(port);
	printf("%lx and %x\n", to.sin_addr.s_addr, to.sin_port);

    // send the data
    for (int i=0; i<=socket->retries; i++) {
        int ret = lwip_sendto(socket->state, buf, len, 0, (struct sockaddr*)&to, sizeof(to));
        if (ret > 0)
			return ret;
        if (ret == -1 && errno != EWOULDBLOCK) {
			*_errno = errno;
			return -1;
        }
    }
    return MP_ETIMEDOUT; 
}

STATIC mp_uint_t lan_socket_recvfrom(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len, byte *ip, mp_uint_t *port, int *_errno) {
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

	memset(&from, 0x0, fromlen);
	mp_uint_t ret = _socket_read_data(socket, buf, len, (struct sockaddr *)&from, &fromlen, _errno);

    *port = lwip_ntohs(from.sin_port);
    int32_t i32IPAddr = from.sin_addr.s_addr;
    ip[3] = (byte)((i32IPAddr >> 24) & 0xFF);
    ip[2] = (byte)((i32IPAddr >> 16) & 0xFF);
    ip[1] = (byte)((i32IPAddr >> 8) & 0xFF);
    ip[0] = (byte)(i32IPAddr & 0xFF);

    return ret;
}

STATIC int lan_socket_setsockopt(mod_network_socket_obj_t *socket, mp_uint_t level, mp_uint_t opt, const void *optval, mp_uint_t optlen, int *_errno) {

    switch (opt) {
        // level: SOL_SOCKET
        case SO_REUSEADDR: {
            int ret = lwip_setsockopt(socket->state, SOL_SOCKET, opt, &optval, optlen);
            if (ret != 0) {
				*_errno = errno;
				return -1;
            }
            break;
        }
        default: {
            mp_printf(&mp_plat_print, "Warning: lwip.setsockopt() option not implemented\n");
			return -2;
		}
    }

	return 0;
}

STATIC int lan_socket_settimeout(mod_network_socket_obj_t *socket, mp_uint_t timeout_ms, int *_errno) {

	_socket_settimeout(socket, timeout_ms);
    return 0;
}

STATIC int lan_socket_ioctl(mod_network_socket_obj_t *socket, mp_uint_t request, mp_uint_t arg, int *_errno) {

    if (request == MP_STREAM_POLL) {

        fd_set rfds; FD_ZERO(&rfds);
        fd_set wfds; FD_ZERO(&wfds);
        fd_set efds; FD_ZERO(&efds);
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
        if (arg & MP_STREAM_POLL_RD) FD_SET(socket->state, &rfds);
        if (arg & MP_STREAM_POLL_WR) FD_SET(socket->state, &wfds);
        if (arg & MP_STREAM_POLL_HUP) FD_SET(socket->state, &efds);

        int r = select((socket->state)+1, &rfds, &wfds, &efds, &timeout);
        if (r < 0) {
            *_errno = MP_EIO;
            return MP_STREAM_ERROR;
        }

        mp_uint_t ret = 0;
        if (FD_ISSET(socket->state, &rfds)) ret |= MP_STREAM_POLL_RD;
        if (FD_ISSET(socket->state, &wfds)) ret |= MP_STREAM_POLL_WR;
        if (FD_ISSET(socket->state, &efds)) ret |= MP_STREAM_POLL_HUP;
        return ret;
    } else if (request == MP_STREAM_CLOSE) {
        if (socket->state >= 0) {
            #if MICROPY_PY_USOCKET_EVENTS
            if (socket->events_callback != MP_OBJ_NULL) {
                usocket_events_remove(socket);
                socket->events_callback = MP_OBJ_NULL;
            }
            #endif
            int ret = lwip_close(socket->state);
            if (ret != 0) {
                *_errno = errno;
                return MP_STREAM_ERROR;
            }
            socket->state = -1;
        }
        return 0;
    }

    *_errno = MP_EINVAL;
    return MP_STREAM_ERROR;
}

const mod_network_nic_type_t mod_network_nic_type_lan = {
    .base = {
        { &mp_type_type },
        .name = MP_QSTR_LAN,
        .make_new = lan_if_make_new,
        .locals_dict = (mp_obj_dict_t*)&lan_if_locals_dict,
    },

	.gethostbyname = lan_gethostbyname,
    .socket = lan_socket_socket,
    .close = lan_socket_close,
    .bind = lan_socket_bind,
    .connect = lan_socket_connect,
    .listen = lan_socket_listen,
    .accept = lan_socket_accept,
    .send = lan_socket_send,
    .recv = lan_socket_recv,
    .sendto = lan_socket_sendto,
    .recvfrom = lan_socket_recvfrom,
    .setsockopt = lan_socket_setsockopt,
    .settimeout = lan_socket_settimeout,
    .ioctl = lan_socket_ioctl,

};

#endif
