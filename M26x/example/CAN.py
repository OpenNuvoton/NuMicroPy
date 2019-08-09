import pyb
from pyb import CAN

buf = bytearray(8)
data_lst = [0, 0, 0, memoryview(buf)]
can_write = 0

def can_cb(bus, reason, fifo_num):
        if reason == CAN.CB_REASON_RX:
                bus.recv(fifo = fifo_num, list = data_lst)
                can_write = 1
                print(buf)
        if reason == CAN.CB_REASON_ERROR_WARNING:
                print('Error Warning')
        if reason == CAN.CB_REASON_ERROR_PASSIVE:
                print('Error Passive')
        if reason == CAN.CB_REASON_ERROR_BUS_OFF:
                print('Bus off')

can0 = CAN(0, mode = CAN.NORMAL, extframe=True, baudrate = 500000)
can0.setfilter(id = 0x12345, fifo = 10, mask = 0xFFFF0)
can0.rxcallback(can_cb)

while True:
	pyb.delay(500)
	if can_write == 1:
		can0.send(data = 'ABC', id = 0x567)
		can_write = 0
