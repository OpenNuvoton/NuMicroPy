from pyb import UART

uart = UART(6, 115200, bits = 8, parity = None, stop = 1)

uart.writechar(0x55)
uart.writechar(0xaa)
uart.writechar(0x55)
uart.writechar(0xaa)

uart.write('\r\n')
uart.write('>>')

read_char = 0

while (read_char != 113):			#exit if received 'q'
	if(uart.any() != 0):
		read_char = uart.readchar()
		print(read_char)
		uart.writechar(read_char)
