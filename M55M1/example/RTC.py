import pyb
rtc = pyb.RTC()
rtc.datetime((2025, 6, 13, 5, 14, 30, 0, 0))
print(rtc.datetime())

def rtc_cb():
	print("wake up")

rtc.wakeup(rtc.WAKEUP_1_SEC, rtc_cb)
