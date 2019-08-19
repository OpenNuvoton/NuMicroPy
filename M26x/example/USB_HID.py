import pyb

pyb.usb_mode('HID')
hid = pyb.USB_HID()

#Prepare multiple of 64 bytes buffer for sending or receiving, beacuse each HID transaction is 64 bytes
send_pakcet_size = hid.send_packet_size()
send_buf = bytearray(send_pakcet_size)
recv_pakcet_size = hid.recv_packet_size()
recv_buf = bytearray(recv_pakcet_size)

send_buf[0] = 1
hid.send(send_buf)
hid.recv(recv_buf)

pyb.delay(1000)
send_buf[0] = recv_buf[0] + 1
hid.send(send_buf)
hid.recv(recv_buf)

pyb.delay(1000)
send_buf[0] = recv_buf[0] + 1
hid.send(send_buf)
hid.recv(recv_buf)

pyb.delay(1000)
send_buf[0] = recv_buf[0] + 1
hid.send(send_buf)
hid.recv(recv_buf)
