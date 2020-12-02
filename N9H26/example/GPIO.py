from pyb import Pin

#Set pin to alterate function
#a3 = Pin(Pin.board.GPA3, Pin.ALT, alt = Pin.AF_PA3_KPI0_SI0)
#print(a3)

#For GPIO output
#a3 = Pin(Pin.board.GPA3, Pin.OUT)
#print(a3)
#a3.value(1)
count = 0

def pa3_callback(pin):
	count = count + 1
	
#For GPIO input
a3 = Pin(Pin.board.GPA3, Pin.IN)
a3.irq(handler=pa3_callback, trigger=Pin.IRQ_RISING)

while True:
	print(count)
	pyb.delay(2000)

