from pyb import LED

rgb = LED(LED.RGB)
rgb.rgb_write(7, 255, 0, 0)
rgb.rgb_write(6, 0, 255, 0)
rgb.rgb_write(5, 0, 0, 255)

a=[[1,0,0],[0,0,10],[20,0,0],[0,40,0],[0,0,80]]
rgb.rgb_write(a)
