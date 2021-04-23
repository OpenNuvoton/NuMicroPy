# Enable/disable modules and 3rd-party libs to be included in interpreter

# _thread module using pthreads
MICROPY_PY_THREAD = 1

#LittlevGL binding
MICROPY_LVGL = 1

ifeq ($(MICROPY_PY_THREAD),1)
# socket module using lwip. depend on MICROPY_PY_THREAD
	ifeq ($(BOARD),NuMaker-IOT-M487)
		MICROPY_WLAN_ESP8266 = 1
		MICROPY_NVT_LWIP = 0
	else
		MICROPY_WLAN_ESP8266 = 0
		MICROPY_NVT_LWIP = 1
	endif
endif

#MICROPY_SSL_NVT_MBEDTLS = 1
MICROPY_SSL_NVT_AXTLS = 1

