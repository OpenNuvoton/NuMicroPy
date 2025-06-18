import math
from array import array
from pyb import DAC
import time


dac = DAC(0, bits = 8)
dac.write(0)
dac.write(64)
dac.write(128)
dac.write(172)
dac.write(255)
dac.deinit()

# create a buffer containing a sine-wave, using half-word samples
buf = array('H', 2048 + int(2047 * math.sin(2 * math.pi * i / 128)) for i in range(128))

#reinit dac to 8bit mode
dac = DAC(0, bits = 12)

# output the sine-wave at 400Hz
dac.write_timed(buf, 400 * len(buf), mode=DAC.CIRCULAR)
time.sleep_ms(5000)
dac.deinit()

