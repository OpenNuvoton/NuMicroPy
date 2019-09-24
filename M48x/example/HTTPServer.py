import network
import usocket as socket
import pyb

pins = [pyb.Pin('SW2', pyb.Pin.IN), pyb.Pin('SW3', pyb.Pin.IN)]

netif = 'WLAN'

if netif == "LAN":
	lan = network.LAN()

	while True:
		if(lan.ifconfig("dhcp") == True):
			break
	print(lan.ifconfig())
else:
	wlan = network.WLAN()
	wlan.connect("NT_ZY", "12345678")
	print(wlan.ifconfig())

html = """<!DOCTYPE html>
<html>
    <head> <title>M487 Pins</title> </head>
    <body> <h1>M480 Pins</h1>
        <table border="1"> <tr><th>Pin</th><th>Value</th></tr> %s </table>
    </body>
</html>
"""

addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]

s = socket.socket()
s.bind(addr)
s.listen(1)

print('listening on', addr)

while True:
    cl, addr = s.accept()
    print('client connected from', addr)
    cl_file = cl.makefile('rwb', 0)
    while True:
        line = cl_file.readline()
        if not line or line == b'\r\n':
            break
    rows = ['<tr><td>%s</td><td>%d</td></tr>' % (str(p), p.value()) for p in pins]
    response = html % '\n'.join(rows)
    cl.send(response)
    cl.close()
