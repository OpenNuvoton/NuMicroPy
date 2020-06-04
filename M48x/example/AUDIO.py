import pyb
from umachine import AUDIO

aud = AUDIO()

print("Play 44-320.mp3")
aud.mp3_play("/sd/44-320.mp3")

#Wait play stop
while(aud.mp3_status() != aud.STATUS_STOP):
	pyb.delay(1000)
	
print("End play")
pyb.delay(1000)
print("Play 16-128.mp3")
aud.mp3_play("/sd/16-128.mp3")
pyb.delay(20000)
print("Stop play")
aud.mp3_stop()

