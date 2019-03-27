#Setup micropython and M480 BSP path
MICROPY_SRC_PATH = ../micropython
M480_BSP_PATH = ../M480BSP

# Select the board to build for: if not given on the command line,
# then default to PYBV10.
BOARD ?= NuMaker-PFM-M487
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)

#Extra clean directory
CLEAN_EXTRA = M480BSP

-include mpconfigport.mk
include $(MICROPY_SRC_PATH)/py/mkenv.mk
include boards/$(BOARD)/mpconfigboard.mk

CROSS = 1

M48XBSP_LIB = $(M480_BSP_PATH)/Library
M48XBSP_THIRD_PARTY = $(M480_BSP_PATH)/ThirdParty
STARTUP_FILE = $(M48XBSP_LIB)/Device/Nuvoton/M480/Source/GCC/startup_M480.o

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h $(BUILD)/pins_qstr.h

# include py core make definitions
include $(TOP)/py/py.mk

ifeq ($(MICROPY_SSL_NVT_MBEDTLS),1)
SSL_MBEDTLS_INCLUDE = $(TOP)/$(M48XBSP_THIRD_PARTY)/mbedtls-2.13.0/include
CFLAGS_MOD += -DMICROPY_SSL_MBEDTLS=1 -DMICROPY_PY_USSL=1 -DMICROPY_PY_UCRYPTOLIB=1 -I$(SSL_MBEDTLS_INCLUDE)
endif

ifeq ($(MICROPY_SSL_NVT_AXTLS),1)
SSL_AXTLS_INCLUDE = lib/axtls
CFLAGS_MOD += -DMICROPY_SSL_AXTLS=1 -DMICROPY_PY_USSL=1 -DMICROPY_PY_UCRYPTOLIB=1 -I$(SSL_AXTLS_INCLUDE)/ssl -I$(SSL_AXTLS_INCLUDE)/crypto -I$(TOP)/extmod/axtls-include
endif

ifeq ($(CROSS), 1)
CROSS_COMPILE = arm-none-eabi-
endif

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(BUILD)/genhdr
INC += -Iboards/$(BOARD)
INC += -I$(TOP)/$(M48XBSP_LIB)/Device/Nuvoton/M480/Include
INC += -I$(TOP)/$(M48XBSP_LIB)/CMSIS/Include
INC += -I$(TOP)/$(M48XBSP_LIB)/StdDriver/inc
INC += -I$(TOP)/$(M48XBSP_THIRD_PARTY)
INC += -I$(TOP)/$(M48XBSP_THIRD_PARTY)/FreeRTOS
INC += -I$(TOP)/$(M48XBSP_THIRD_PARTY)/FreeRTOS/Source/include
INC += -I$(TOP)/$(M48XBSP_THIRD_PARTY)/FreeRTOS/Source/portable/GCC/ARM_CM4F

ifeq ($(CROSS), 1)
CFLAGS_CORTEX_M4 = -mthumb -mtune=cortex-m4 -mabi=aapcs -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant -Wdouble-promotion
CFLAGS = $(INC) -Wall -std=c99 $(CFLAGS_CORTEX_M4) $(CFLAGS_MOD) $(COPT)
LD = $(CC)
#LDFLAGS = -nostdlib -T gcc_arm.ld -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections -Wl,-no-enum-size-warning
LDFLAGS = $(addprefix -T,$(LD_FILES)) $(CFLAGS_CORTEX_M4)  -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections
else
LD = gcc
CFLAGS = -m32 $(INC) -Wall -Werror -std=c99 $(COPT)
LDFLAGS = -m32 -Wl,-Map=$@.map,--cref -Wl,--gc-sections
endif

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

LIBS =

SRC_C = \
	main.c \
	gccollect.c \
	fatfs_port.c \
	bufhelper.c \
	misc/FreeRTOSHooks.c \
	misc/mperror.c \
	mpthreadport.c \
	$(BUILD)/_frozen_mpy.c \
	$(wildcard boards/$(BOARD)/*.c)


SRC_LIB_C = $(addprefix lib/,\
	utils/pyexec.c \
	libc/string0.c \
	oofatfs/ff.c \
	oofatfs/option/unicode.c \
	mp-readline/readline.c \
	timeutils/timeutils.c \
	netutils/netutils.c \
	utils/sys_stdio_mphal.c \
	)

SRC_DRIVER_C = $(addprefix drivers/,\
	bus/softspi.c \
	)

SRC_MODS_C = $(addprefix mods/,\
	modmachine.c \
	modpyb.c \
	modnetwork.c \
	pybsdcard.c \
	pybpin.c \
	pin_named_pins.c \
	pybirq.c \
	pybi2c.c \
	pybspi.c \
	pybadc.c \
	networklan.c \
	modsocket.c \
	pybrtc.c \
	modutime.c \
	pybtimer.c \
	pybuart.c \
	pybdac.c \
	pybcan.c \
	pybusb.c \
	pybflash.c \
	pybswitch.c \
	pybled.c \
	moduos.c \
	rng.c \
	pybpwm.c \
	pybwdt.c \
	)

SRC_O = \
	$(STARTUP_FILE) \
	gchelper.o \

SRC_SYS_C = $(M48XBSP_LIB)/Device/Nuvoton/M480/Source/system_M480.c \

SRC_BSP_DRIVER_C = $(addprefix $(M48XBSP_LIB)/StdDriver/src/,\
	retarget.c \
	clk.c \
	uart.c \
	sdh.c \
	gpio.c \
	i2c.c \
	spi.c \
	pdma.c \
	sys.c \
	eadc.c\
	rtc.c \
	timer.c \
	timer_pwm.c \
	dac.c \
	can.c \
	usbd.c \
	fmc.c \
	crypto.c \
	bpwm.c \
	epwm.c \
	wdt.c \
	)

ifeq ($(MICROPY_PY_THREAD),1)

CFLAGS += -DMICROPY_PY_THREAD=1

SRC_RTOS_C = $(addprefix $(M48XBSP_THIRD_PARTY)/FreeRTOS/Source/,\
	list.c \
	queue.c \
	tasks.c \
	timers.c \
	portable/GCC/ARM_CM4F/port.c \
	portable/MemMang/heap_4.c \
	)

endif

ifeq ($(MICROPY_SSL_NVT_MBEDTLS),1)

SRC_MBEDTLS_C = $(addprefix $(M48XBSP_THIRD_PARTY)/mbedtls-2.13.0/library/,\
	aes.c \
	sha256.c \
	platform_util.c \
	pk.c \
	sha1.c \
	sha512.c \
	pk_wrap.c \
	md_wrap.c \
	cipher_wrap.c \
	ssl_tls.c \
	ssl_cli.c \
	ssl_srv.c \
	debug.c \
	x509_crt.c \
	md.c \
	asn1parse.c \
	cipher.c \
	ctr_drbg.c \
	md5.c \
	x509.c \
	entropy.c \
	pkparse.c \
	rsa.c \
	ecp.c \
	bignum.c \
	ecdsa.c \
	ripemd160.c \
	ssl_ciphersuites.c \
	dhm.c \
	ecdh.c \
	pem.c \
	gcm.c \
	ecp_curves.c \
	oid.c \
	chachapoly.c \
	chacha20.c \
	des.c \
	ccm.c \
	camellia.c \
	blowfish.c \
	arc4.c \
	entropy_poll.c \
	pkcs12.c \
	pkcs5.c \
	rsa_internal.c \
	hmac_drbg.c \
	asn1write.c \
	base64.c \
	poly1305.c \
	)

endif

ifeq ($(MICROPY_SSL_NVT_AXTLS),1)

SRC_AXTLS_C = $(addprefix lib/axtls/,\
	ssl/asn1.c \
	ssl/loader.c \
	ssl/tls1.c \
	ssl/tls1_svr.c \
	ssl/tls1_clnt.c \
	ssl/x509.c \
	crypto/aes.c \
	crypto/bigint.c \
	crypto/crypto_misc.c \
	crypto/hmac.c \
	crypto/md5.c \
	crypto/rsa.c \
	crypto/sha1.c \
	crypto/sha256.c \
	crypto/sha512.c \
	crypto/sha384.c \
	)

endif

ifeq ($(MICROPY_NVT_LWIP),1)
INC += -I$(TOP)/$(M48XBSP_THIRD_PARTY)/lwIP/src/include -Ilwip-port/include
CFLAGS += -DMICROPY_NVT_LWIP=1

SRC_LWIP_C = lwip-port/netif/ethernetif.c lwip-port/sys_arch.c
SRC_LWIP_C += $(addprefix $(M48XBSP_THIRD_PARTY)/lwIP/src/,\
	core/def.c \
	core/dns.c \
	core/init.c \
	core/mem.c \
	core/memp.c \
	core/netif.c \
	core/pbuf.c \
	core/raw.c \
	core/stats.c \
	core/tcp.c \
	core/tcp_in.c \
	core/tcp_out.c \
	core/udp.c \
	core/ip.c \
	core/timeouts.c \
	core/ipv4/autoip.c \
	core/ipv4/icmp.c \
	core/ipv4/igmp.c \
	core/inet_chksum.c \
	core/ipv4/ip4_addr.c \
	core/ipv4/ip4.c \
	core/ipv4/ip4_frag.c \
	core/ipv4/dhcp.c \
	core/ipv4/etharp.c \
	netif/ethernet.c \
	api/tcpip.c \
	api/api_lib.c \
	api/api_msg.c \
	api/sockets.c \
	api/netbuf.c \
	api/netdb.c \
	api/err.c \
	)
endif

SRC_HAL_C = $(addprefix hal/,\
	mphalport.c \
	SDCard.c \
	m48x_ISR.c \
	pin_defs_m48x.c \
	pin_int.c \
	systick.c \
	M48x_I2C.c \
	nu_modutil.c \
	M48x_SPI.c \
	M48x_EADC.c\
	dma.c \
	M48x_ETH.c \
	M48x_USBD.c \
	HID_VCPTrans.c \
	HID_VCPDesc.c \
	MSC_Desc.c \
	MSC_Trans.c \
	StorIF_Flash.c \
	)

OBJ =
OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIB_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_MODS_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_SYS_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_DRIVER_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_BSP_DRIVER_C:.c=.o))
ifeq ($(MICROPY_PY_THREAD),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_RTOS_C:.c=.o))
endif
ifeq ($(MICROPY_SSL_NVT_MBEDTLS),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_MBEDTLS_C:.c=.o))
endif
ifeq ($(MICROPY_SSL_NVT_AXTLS),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_AXTLS_C:.c=.o))
endif
ifeq ($(MICROPY_NVT_LWIP),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_LWIP_C:.c=.o))
endif
OBJ += $(addprefix $(BUILD)/, $(SRC_HAL_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_O))
OBJ += $(BUILD)/pins_$(BOARD).o

SRC_QSTR += $(SRC_MODS_C) $(EXTMOD_SRC_C) $(SRC_LIB_C)

all: $(BUILD)/firmware.bin

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin
	cp $(BUILD)/firmware.bin ../build/NuMaker-PFM-M487/

test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"


MAKE_PINS = boards/make-pins.py
BOARD_PINS = boards/$(BOARD)/pins.csv
PREFIX_FILE = boards/m48x_prefix.c
GEN_PINS_SRC = $(BUILD)/pins_$(BOARD).c
GEN_PINS_HDR = $(HEADER_BUILD)/pins.h
GEN_PINS_QSTR = $(BUILD)/pins_qstr.h
GEN_PINS_AF_CONST = $(HEADER_BUILD)/pins_af_const.h


# Making OBJ use an order-only depenedency on the generated pins.h file
# has the side effect of making the pins.h file before we actually compile
# any of the objects. The normal dependency generation will deal with the
# case when pins.h is modified. But when it doesn't exist, we don't know
# which source files might need it.
$(OBJ): | $(GEN_PINS_HDR)

# With conditional pins, we may need to regenerate qstrdefs.h when config
# options change.
#$(HEADER_BUILD)/qstrdefs.generated.h: boards/$(BOARD)/mpconfigboard.h

# Use a pattern rule here so that make will only call make-pins.py once to make
# both pins_$(BOARD).c and pins.h
# $(BUILD)/%_$(BOARD).c $(HEADER_BUILD)/%.h $(BUILD)/%_qstr.h: boards/$(BOARD)/%.csv $(MAKE_PINS) $(AF_FILE) $(PREFIX_FILE) | $(HEADER_BUILD)
$(GEN_PINS_SRC) $(GEN_PINS_HDR) $(GEN_PINS_QSTR) $(GEN_PINS_AF_CONST): $(BOARD_PINS) $(MAKE_PINS) $(AF_FILE) $(PREFIX_FILE) | $(HEADER_BUILD)
	$(ECHO) "GEN $@"
	$(Q)$(PYTHON) $(MAKE_PINS) --board $(BOARD_PINS) --af $(AF_FILE) --af-const $(GEN_PINS_AF_CONST) --prefix $(PREFIX_FILE) --hdr $(GEN_PINS_HDR) --qstr $(GEN_PINS_QSTR) > $(GEN_PINS_SRC)

$(BUILD)/pins_$(BOARD).o: $(BUILD)/pins_$(BOARD).c
	$(call compile_c)

include $(TOP)/py/mkrules.mk


