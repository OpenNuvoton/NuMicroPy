import network
import usocket as socket

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

url = 'http://micropython.org/ks/test.html'
_, _, host, path = url.split('/', 3)
addr = socket.getaddrinfo(host, 80)[0][-1]
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(addr)
#nuvoton proxy server
#s.connect(("10.254.239.51", 8080))
s.send(bytes('GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n' % (path, host), 'utf8'))
while True:
	data = s.recv(100)
	if data:
		print(str(data, 'utf8'), end='')
	else:
		break
s.close()
