import AudioIn as ain
import sensor as snr
import VPOST as VP
import Record as rec

#Enable audio-in device
ain.init(channels=1, frequency=16000)

#Enable sensor device
snr.reset()
snr.set_pixformat(snr.YUV420MB, snr.YUV422)
snr.set_framesize(snr.VGA, snr.QVGA)

#Enable VPOST frame layer
disp = VP.display(ColorFormat=VP.YCBYCR)
disp.init()

#Create video-in. audio-in and video-out(for preview) device driver type
vin_drv = rec.vin_drv_t()
ain_drv = rec.ain_drv_t()
vout_drv = rec.vout_drv_t()

#Setup and register video-in driver
vin_drv.width = 640
vin_drv.height = 480
vin_drv.pixel_format = rec.YUV420MB
vin_drv.planar_img_fill_cb=snr.planar_fill
vin_drv.packet_img_fill_cb=snr.packet_fill
rec.vin_drv_register(vin_drv)

#Setup and register audio-in driver
ain_drv.samplerate = 16000
ain_drv.channel = 1
ain_drv.fill_cb=ain.read_frame
rec.ain_drv_register(ain_drv)

#Setup and register video-out driver
vout_drv.flush_cb=disp.render
vout_drv.vout_obj=disp
rec.vout_drv_register(vout_drv)

#record init and enable preview
rec.init(True)

#start record
rec.start_record("/sd/DCIM")

#record 30 sec
pyb.delay(30000)

#stop record
rec.stop_record()


