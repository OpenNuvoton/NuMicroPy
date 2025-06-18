import time
from pyb import Pin

def btn1_callback(pin):
	print(pin)

btn1 = Pin.board.BTN1
btn1_af = btn1.af_list()

#btn1 already in input mode
print(btn1)
print(btn1.af_list())

#set pin to alterate function
#d1 = Pin(Pin.board.D1, Pin.ALT, alt = 0)
#d1 = Pin(Pin.board.D1, Pin.ALT, alt = Pin.AF_PB3_EADC0_CH3)

btn1.irq(handler=btn1_callback, trigger=Pin.IRQ_FALLING)

#change ledr to output mode
r = Pin('LEDR', Pin.OUT)
#r = Pin('LEDR', Pin.OUT, pull = None, value = 0)	#Defalut output low

print(r)
print(r.af_list())

while True:
	pin_value = btn1.value()
	if pin_value == 0:
		print('Button press')
		time.sleep_ms(1000)

print('demo done')
