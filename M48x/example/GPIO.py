from pyb import Pin

def sw2_callback(pin):
	print(pin)

sw2 = Pin.board.SW2
sw2_af = sw2.af_list()

#sw2 already in inport mode
print(sw2)
print(sw2.af_list())

#set pin to alterate function
#d1 = Pin(Pin.board.D1, Pin.ALT, alt = 0)
#d1 = Pin(Pin.board.D1, Pin.ALT, alt = Pin.AF_PB3_EADC0_CH3)


sw2.irq(handler=sw2_callback, trigger=Pin.IRQ_RISING)

#change ledr to output mode
r = Pin('LEDR', Pin.OUT)
#r = Pin('LEDR', Pin.OUT, pull = None, value = 0)	#Defalut output low

print(r)
print(r.af_list())

while True:
	pin_value = sw2.value()
	if pin_value == 0:
		print('key press')
		break
print('demo done')
