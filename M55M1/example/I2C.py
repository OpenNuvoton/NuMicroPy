#master mode
from pyb import I2C
i2c3 = I2C(3, I2C.MASTER)
slave_addr = i2c3.scan()
print(slave_addr)
i2c3.send('123', slave_addr[0])
data = bytearray(3)
i2c3.recv(data, slave_addr[0])
print(data)

#slave mode
from pyb import I2C
i2c3 = I2C(3, I2C.SLAVE, addr=0x15)
recv_data = bytearray(3)
send_data = bytearray(1)
i2c3.recv(recv_data)
print(recv_data)
send_data[0] = recv_data[2]
i2c3.send(send_data)
