import pyb

pyb.usb_mode('VCP')
vcp = pyb.USB_VCP()

send_pakcet_size = 1
send_buf = bytearray(send_pakcet_size)
recv_pakcet_size = 64
recv_buf = bytearray(recv_pakcet_size)

while True:
	if vcp.any() !=0:
		recv_len = vcp.recv(recv_buf, timeout = 100)
		for i in range(recv_len):
			send_buf[0] = recv_buf[i]
			vcp.send(send_buf)

