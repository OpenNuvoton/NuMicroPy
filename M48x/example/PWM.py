from pyb import PWM
from pyb import Pin

from pyb import PWM
from pyb import Pin

def capture_cb(chan, reason):
	if reason == PWM.RISING:
		print("rising", chan.capture(PWM.RISING))
	elif reason == PWM.FALLING:
		print('falling', chan.capture(PWM.FALLING))
	else:
		print('both edge')

bpwm1 = PWM(1, freq = 1000)
epwm0 = PWM(0, PWM.EPWM, freq = 1000000)

bpwm1ch4 = bpwm1.channel(mode = PWM.OUTPUT, pulse_width_percent = 25, pin = Pin.board.A1)
epwm0ch1 = epwm0.channel(mode = PWM.CAPTURE, capture_edge = PWM.RISING_FALLING, pin = Pin.board.D9, callback = capture_cb)
