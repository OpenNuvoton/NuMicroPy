from pyb import Pin

def d0_callback(pin):
	print(pin)

d0 = Pin.board.D0
d0_af = d0.af_list()

print(d0)
print(d0.af_list())

#set pin to alterate function
#d1 = Pin(Pin.board.D1, Pin.ALT, alt = 0)
#d1 = Pin(Pin.board.D1, Pin.ALT, alt = Pin.AF_PB3_EADC0_CH3)

d0.irq(handler=d0_callback, trigger=Pin.IRQ_RISING)

while True:
	pin_value = d0.value()
	if pin_value == 0:
		print('d0 is low')
		break
print('demo done')
