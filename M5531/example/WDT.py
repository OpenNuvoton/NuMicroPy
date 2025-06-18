import utime
from pyb import WDT
import time

wdt = WDT(timeout = 1000)

while utime.ticks_ms() < 10000:
	time.sleep_ms(500)
	wdt.feed()

print('Ready to reset')
