from pyb import PWM
from pyb import Pin

#connect A1 and D9 board pin

def capture_cb(chan, reason):
	if reason == PWM.RISING:
		print('rising edge')
	elif reason == PWM.FALLING:
		print('falling edge')
	else:
		print('both edge')

#BPWM
bpwm1 = PWM(1, freq = 2)
#EPWM
epwm0 = PWM(0, PWM.EPWM, freq = 8)

#board pin A1, CPU pin B7
bpwm1ch4 = bpwm1.channel(mode = PWM.OUTPUT, pulse_width_percent = 50, pin = Pin.board.A1)

#board pin D9, CPU pin A4
epwm0ch1 = epwm0.channel(mode = PWM.CAPTURE, capture_edge = PWM.RISING, pin = Pin.board.D9, callback = capture_cb)
