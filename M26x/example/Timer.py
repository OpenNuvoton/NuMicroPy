from pyb import Pin
from pyb import Timer

def tick(timer):
        print(timer.counter())

#tim = Timer(3, freq = 2, callback = tick)
tim = Timer(3, freq = 2)
tim.callback(tick)

#PWM test
chan = tim.channel(Timer.PWM, pin = Pin.board.D0, pulse_width_percent = 20)

#output toggle test
chan = tim.channel(Timer.OC_TOGGLE, pin = Pin.board.D0)

#input capture test
chan = tim.channel(Timer.IC, pin = Pin.board.D0, polarity = Timer.RISING)
chan.callback(tick)
