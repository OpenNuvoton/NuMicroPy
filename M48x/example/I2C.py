#master device
from pyb import I2C
i2c0 = I2C(0, I2C.MASTER)
slave_addr = i2c0.scan()
i2c0.send('123', slave_addr[0])
data = bytearray(3)
i2c0.recv(data, slave_addr[0])

#slave device
from pyb import I2C
i2c0 = I2C(0, I2C.SLAVE, addr=0x12)
data = bytearray(4)
i2c0.recv(data)
data[0]=data[0]+1
data[1]=data[1]+1
data[2]=data[2]+1
i2c0.send(data)
