import AudioIn
import SPU
import pyb

AudioIn.init(channels=1, frequency=16000)
frag_size = SPU.init(channels=1, frequency=16000, volume=70)
pcm_data=bytearray(frag_size)

def audioin_cb(ain_data):
	pcm_data.extend(ain_data)

AudioIn.start_streaming(audioin_cb)
SPU.start_play(None)

while True:
	if(len(pcm_data) > 0):
		put_len=SPU.put_frame(pcm_data)
		if(put_len > 0):
			pcm_data=pcm_data[put_len:]
			
AudioIn.stop_streaming()
SPU.stop_play()
