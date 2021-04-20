import pyb
rtc = pyb.RTC()
rtc.datetime((2014, 5, 1, 4, 13, 0, 0, 0))
print(rtc.datetime())

def rtc_cb()
	print("wake up")

rtc.wakeup(rtc.WAKEUP_1_SEC, rtc_cb)
