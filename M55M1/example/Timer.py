from pyb import Pin
from pyb import Timer
from pyb import PWM

##############################################################
##	General timer function
##############################################################
def tick(timer):
	print(timer.counter())

tim = Timer(3, freq = 2000)
#tim.callback(tick)

#PWM channel test
chan = tim.channel(Timer.PWM, pin = Pin.board.D0, pulse_width_percent = 20)

#output toggle channel test
chan = tim.channel(Timer.OC_TOGGLE, pin = Pin.board.D0)

##############################################################
##	Timer input capture function
##############################################################
def tick(chan):
	print(chan.capture())

tim = Timer(3, freq = 100000)

#source signal: NuMaker baord UNO A1 pin
bpwm1 = PWM(1, freq = 2)
bpwm1ch4 = bpwm1.channel(mode = PWM.OUTPUT, pulse_width_percent = 50, pin = Pin.board.A1)

#capture pin: NuMaker board UNO MOSI ---> A1
chan = tim.channel(Timer.IC, pin = Pin.board.MOSI, polarity = Timer.RISING)
chan.callback(tick)
