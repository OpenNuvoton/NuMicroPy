import pyb

def sw2_callback():
	print('sw2 press')
	
def sw3_callback():
	print('sw3 press')

sw2 = pyb.Switch('sw2')
sw3 = pyb.Switch('sw3')

sw2.callback(sw2_callback)
sw3.callback(sw3_callback)

while True:
	pyb.delay(1000)
