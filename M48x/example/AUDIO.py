import pyb
from umachine import AUDIO

pyb.msc_disable()

aud = AUDIO()

print("Start record file")
afile = aud.wav_record(file="/sd/qqq.wav")
pyb.delay(30000)
aud.wav_stop()

pyb.delay(1000)

print("Play recorded file")
aud.wav_play(afile)

#Wait play stop
while(aud.wav_status() != aud.STATUS_STOP):
	pyb.delay(1000)

print("End play")

pyb.msc_enable()

aud.wav_play("/sd/Left_Right_stereo_44_PCM.wav")
