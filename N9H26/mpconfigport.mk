# Enable/disable modules and 3rd-party libs to be included in interpreter

# _thread module using pthreads
MICROPY_PY_THREAD = 1

# enable LWIP for ethernet build
MICROPY_NVT_LWIP = 1

#LittlevGL binding
MICROPY_LVGL = 1

#openmv
MICROPY_OPENMV = 1

#POSIX
MICROPY_POSIX = 1

#NVT media
ifeq ($(MICROPY_POSIX),1)
MICROPY_NVTMEDIA = 1
endif
