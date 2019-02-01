import pyb

r = pyb.Pin('LEDR', pyb.Pin.OUT)
g = pyb.Pin('LEDG', pyb.Pin.OUT)
y = pyb.Pin('LEDY', pyb.Pin.OUT)

while True:
	pyb.delay(1000)
	pin_value = r.value()
	if pin_value == 0:
		r.value(1)
	else:
		r.value(0)
	pyb.delay(1000)
	pin_value = y.value()
	if pin_value == 0:
		y.value(1)
	else:
		y.value(0)
	pyb.delay(1000)
	pin_value = g.value()
	if pin_value == 0:
		g.value(1)
	else:
		g.value(0)
