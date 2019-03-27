# Enable/disable modules and 3rd-party libs to be included in interpreter

# _thread module using pthreads
MICROPY_PY_THREAD = 1

ifeq ($(MICROPY_PY_THREAD),1)
# socket module using lwip. depend on MICROPY_PY_THREAD
MICROPY_NVT_LWIP = 1
endif

#MICROPY_SSL_NVT_MBEDTLS = 1
MICROPY_SSL_NVT_AXTLS = 1
