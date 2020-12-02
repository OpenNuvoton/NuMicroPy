#Setup micropython and N32926 BSP path
MICROPY_SRC_PATH = ../micropython
N9H26BSP_PATH = ../N9H26_emWin_NonOS/BSP
THIRD_PARTY_PATH = ../ThirdParty

# Select the board to build for: if not given on the command line,
BOARD ?= NHS-N3292x-1-CF
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
#BUILD ?= build-$(BOARD)
BUILD ?= build

#Extra clean directory
CLEAN_EXTRA = N9H26_emWin_NonOS ThirdParty yacctab.py lextab.py

-include mpconfigport.mk
include $(MICROPY_SRC_PATH)/py/mkenv.mk
include boards/$(BOARD)/mpconfigboard.mk

CROSS = 1

N9H26BSP_DRV = $(N9H26BSP_PATH)/Driver
N9H26BSP_THIRD_PARTY = $(N9H26BSP_PATH)/ThirdParty
STARTUP_FILE = Startup.o

# qstr definitions (must come before including py.mk) temp remove $(BUILD)/pins_qstr.h
QSTR_DEFS = qstrdefsport.h $(BUILD)/pins_qstr.h

# include py core make definitions
include $(TOP)/py/py.mk

ifeq ($(CROSS), 1)
CROSS_COMPILE = arm-none-eabi-
endif

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(BUILD)/genhdr
INC += -Iboards/$(BOARD)
INC += -I$(TOP)/$(N9H26BSP_DRV)/Include

ifeq ($(MICROPY_PY_THREAD),1)
INC += -I./lib/FreeRTOS_Port
INC += -I$(TOP)/$(N9H26BSP_THIRD_PARTY)/FreeRTOS
INC += -I$(TOP)/$(N9H26BSP_THIRD_PARTY)/FreeRTOS/Source/include
endif

ifeq ($(CROSS), 1)
CFLAGS_ARM926 = -mcpu=arm926ej-s -mfloat-abi=softfp -fsingle-precision-constant -Wdouble-promotion
CFLAGS = $(INC) -std=c99 -Wall -Wno-unused-function $(CFLAGS_ARM926) $(CFLAGS_MOD) $(COPT)
LD = $(CC)
#LDFLAGS = -nostdlib -T gcc_arm.ld -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections -Wl,-no-enum-size-warning --specs=nosys.specs
LDFLAGS = $(addprefix -T,$(LD_FILES)) $(CFLAGS_ARM926) --specs=nosys.specs --specs=nano.specs -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections
else
LD = gcc
CFLAGS = $(INC) -Wall -Werror -std=c99 $(COPT)
LDFLAGS = -Wl,-Map=$@.map,--cref -Wl,--gc-sections
endif

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -O2 -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

LIBS =

SRC_C = \
	main.c \
	Retarget.c \
	gchelper_generic.c \
	gccollect.c \
	mpthreadport.c \
	fatfs_port.c \
	misc/FreeRTOSHooks.c \
	misc/mperror.c \
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
	utils/interrupt_char.c \
	libm/math.c \
	libm/ef_sqrt.c \
	libm/atan2f.c \
	libm/sf_cos.c \
	libm/sf_sin.c \
	libm/sf_tan.c \
	libm/fmodf.c \
	libm/atanf.c \
	libm/kf_sin.c \
	libm/kf_cos.c \
	libm/kf_tan.c \
	libm/ef_rem_pio2.c \
	libm/kf_rem_pio2.c \
	libm/asinfacosf.c \
	libm/nearbyintf.c \
	)

SRC_MODS_C = $(addprefix mods/,\
	moduos.c \
	modutime.c \
	modmachine.c \
	modpyb.c \
	modnetwork.c \
	modsocket.c \
	pybrtc.c \
	pybspiflash.c \
	pybpin.c \
	pybirq.c \
	pin_named_pins.c \
	networklan.c \
	)

CFLAGS += -D__USBD_STANDARD_MODE__

SRC_BSP_DRIVER_C = $(addprefix $(N9H26BSP_PATH)/Driver/Source/,\
	SYS/wb_uart.c \
	SYS/wb_cache.c \
	SYS/wb_aic.c \
	SYS/wb_mmu.c \
	SYS/wb_config.c \
	SYS/wb_timer.c \
	RTC/RTC.c \
	SPI/spi.c \
	SPI/spiflash_freertos.c \
	USBD/USBD/usbd.c \
	GPIO/libgpio.c \
	)

SRC_HAL_C = $(addprefix hal/,\
	mphalport.c \
	systick.c \
	N9H26_CPSR.c \
	StorIF_SPIFlash.c \
	MSC_VCPDesc.c \
	MSC_VCPTrans.c \
	N9H26_USBDev.c \
	pin_defs_N9H26.c \
	N9H26_ISR.c \
	pin_int.c \
	N9H26_ETH.c \
	)


ifeq ($(MICROPY_PY_THREAD),1)

CFLAGS += -DMICROPY_PY_THREAD=1 -D__FreeRTOS__

SRC_RTOS_C = $(addprefix $(N9H26BSP_THIRD_PARTY)/FreeRTOS/Source/,\
	list.c \
	queue.c \
	tasks.c \
	timers.c \
	portable/MemMang/heap_3.c \
	)

SRC_RTOS_PORT_C = $(addprefix lib/FreeRTOS_Port/,\
	port.c \
	portISR.c \
	)

endif

ifeq ($(MICROPY_NVT_LWIP),1)

INC += -I./lib/lwIP/src/include -Ilwip-port/include
CFLAGS += -DMICROPY_NVT_LWIP=1

SRC_LWIP_C = lwip-port/netif/ethernetif.c lwip-port/sys_arch.c
SRC_LWIP_C += $(addprefix lib/lwIP/src/,\
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


SRC_O = \
	$(STARTUP_FILE) \

OBJ =
OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIB_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_MODS_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_BSP_DRIVER_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_O))
OBJ += $(addprefix $(BUILD)/, $(SRC_HAL_C:.c=.o))
OBJ += $(BUILD)/pins_$(BOARD).o

ifeq ($(MICROPY_PY_THREAD),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_RTOS_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_RTOS_PORT_C:.c=.o))
endif

ifeq ($(MICROPY_NVT_LWIP),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_LWIP_C:.c=.o))
endif

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
	$(Q)$(OBJCOPY) -O binary $^ $(BUILD)/firmware.bin
	cp $(BUILD)/firmware.bin ../build/$(BOARD)/

test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

MAKE_PINS = boards/make-pins.py
BOARD_PINS = boards/$(BOARD)/pins.csv
PREFIX_FILE = boards/N9H26_prefix.c
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