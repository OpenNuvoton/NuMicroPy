import VPOST as VP
import sensor

#Enable frame layer
disp = VP.display(ColorFormat = VP.YCBYCR)
disp.init()

sensor.reset()
sensor.set_pixformat(sensor.YUV422P, sensor.YUV422)
sensor.set_framesize(sensor.VGA, sensor.QVGA)

planar_img = sensor.get_fb(sensor.PIPE_PLANAR)
packet_img = sensor.get_fb(sensor.PIPE_PACKET)

sensor.skip_frames()

while True:
	sensor.snapshot(planar_image=planar_img, packet_image=packet_img)
	disp.render_image(packet_img)

