MCU_SERIES = M48x
CMSIS_MCU = M487
AF_FILE = boards/m487_af.csv
ifeq ($(MICROPY_LVGL),1)
LD_FILES = boards/m487_lvgl.ld
else
LD_FILES = boards/m487.ld
endif
