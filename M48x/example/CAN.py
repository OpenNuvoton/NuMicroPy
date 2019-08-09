from pyb import CAN

buf = bytearray(8)
data_lst = [0, 0, 0, memoryview(buf)]

def can_cb1(bus, reason, fifo_num):
	if reason == CAN.CB_REASON_RX:
		bus.recv(fifo = fifo_num, list = data_lst)
		print(data_lst)
	if reason == CAN.CB_REASON_ERROR_WARNING:
		print('Error Warning')
	if reason == CAN.CB_REASON_ERROR_PASSIVE:
		print('Error Passive')
	if reason == CAN.CB_REASON_ERROR_BUS_OFF:
		print('Bus off')

can0 = CAN(0, mode = CAN.NORMAL, extframe=True, baudrate = 500000)
can1 = CAN(1, mode = CAN.NORMAL, extframe=True, baudrate = 500000)

can1.setfilter(id = 0x55, fifo = 10, mask = 0xf0)
can0.send(data = 'mess 1!', id = 0x50)
can1.recv(fifo = 10)

can1.rxcallback(can_cb1)
can0.send(data = 'mess 2!', id = 0x50)

