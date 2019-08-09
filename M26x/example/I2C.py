from pyb import I2C
i2c0 = I2C(0, I2C.MASTER)
slave_addr = i2c0.scan()
data = bytearray(3)
i2c0.send('123', slave_addr[0])
i2c0.recv(data, slave_addr[0])



