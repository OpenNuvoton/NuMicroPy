import array
import time
from pyb import Pin
from pyb import SPI

send_buf = array.array('i', [0xAA0000, 0xAA0001, 0xAA0002, 0xAA0003, 0xAA0004, 0xAA0005, 0xAA0006, 0xAA0007, 0xAA0008, 0xAA0009, 0xAA000A, 0xAA000B, 0xAA000C, 0xAA000D, 0xAA000E, 0xAA000F])
recv_buf = array.array('i', [0x000000, 0x000001, 0x000002, 0x000003, 0x000004, 0x000005, 0x000006, 0x000007, 0x000008, 0x000009, 0x00000A, 0x00000B, 0x00000C, 0x00000D, 0x00000E, 0xAA000F])
print(send_buf)

#Master code. When SPI slave device ready, press BTN1 to start SPI transfer
btn1 = Pin.board.BTN1
spi0=SPI(0, SPI.MASTER, baudrate=200000, bits=32)

while True:
	pin_value = btn1.value()
	if pin_value == 0:
		print('Start SPI transfer')
		spi0.write_readinto(send_buf, recv_buf)
		print(recv_buf)
		time.sleep_ms(1000)

#Slave
#spi0=SPI(0, SPI.SLAVE, bits=32)
#spi0.write_readinto(send_buf, recv_buf)
#print(recv_buf)
