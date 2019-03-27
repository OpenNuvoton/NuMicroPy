import utime
from pyb import WDT


wdt = WDT(timeout = 1000)

while utime.ticks_ms() < 10000:
	pyb.delay(500)
	wdt.feed()

print('Ready to reset')
