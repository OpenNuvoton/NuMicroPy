from pyb import Accel
from pyb import Gyro
from pyb import Mag

a=Accel(range = Accel.RANGE_8G)
g=Gyro()
m=Mag()

while True:
	pyb.delay(1000)
	ax=a.x()
	ay=a.y()
	az=a.z()
	mx=m.x()
	my=m.y()
	mz=m.z()
	gx=g.x()
	gy=g.y()
	gz=g.z()
	print('-----')
	print(ax,ay,az)
	print(gx,gy,gz)
	print(mx,my,mz)
