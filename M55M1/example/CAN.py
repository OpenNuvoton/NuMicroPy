from pyb import CAN
from pyb import Pin
import time

can0 = CAN(0, mode = CAN.NORMAL, extframe=True, baudrate = 1000000)

#Master, Send Data
can0.send(data = 'mess 01!', id = 0x3333)

#Slave, Receive Data
btn1 = Pin.board.BTN1

buf = bytearray(8)
data_lst = [0, 0, 0, memoryview(buf)]

def can0_cb(bus, reason, fifo_num):
	if reason == CAN.CB_REASON_RX:
		bus.recv(list = data_lst)
		print(data_lst)
		print(buf)
	if reason == CAN.CB_REASON_ERROR_WARNING:
		print('Error Warning')
	if reason == CAN.CB_REASON_ERROR_PASSIVE:
		print('Error Passive')
	if reason == CAN.CB_REASON_ERROR_BUS_OFF:
		print('Bus off')

can0.setfilter(id = 0x333, mask = 0x7FF)
can0.rxcallback(can0_cb)

while True:
	pin_value = btn1.value()
	time.sleep_ms(1000)
	if pin_value == 0:
		print('break main.py')
		break
