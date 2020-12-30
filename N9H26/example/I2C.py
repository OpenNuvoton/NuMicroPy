from pyb import Pin
from pyb import I2C

#Test case
#Device: NT99141 sensor. Must enable pybi2c.c "USE_NT99141_I2C_TEST" option and rebuild firmware

#Set sensor downdown pin low
snr_pw = Pin(Pin.board.SENSOR_POWERDOWN, Pin.OUT, pull = Pin.PULL_UP)
snr_pw.value(0)
pyb.delay(3)

#Reset sensor
snr_reset = Pin(Pin.board.SENSOR_RESET, Pin.OUT, pull = Pin.PULL_UP)
snr_reset.value(1)
pyb.delay(3)
snr_reset.value(0)
pyb.delay(3)
snr_reset.value(1)

#create I2C0 object
slave_addr = 0x54
reg_addr = 0x3009

reg_val = bytearray(1)
print("Buf init value")
print(reg_val[0])

i2c = I2C(0, baudrate = 100)

print("Read test")
i2c.mem_read(reg_val, slave_addr, reg_addr, addr_size=16)
print(reg_val[0])

print("Write test: set value 4")
reg_val[0] = 4
i2c.mem_write(reg_val, slave_addr, reg_addr, addr_size=16)
reg_val[0] = 0
print("Read again")
i2c.mem_read(reg_val, slave_addr, reg_addr, addr_size=16)
print(reg_val[0])
