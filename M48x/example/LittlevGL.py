import lvgl as lv
import ILI9341 as ili
import TouchADC as TP

lv.init()
tp = TP.touch()
tp.init()
disp = ili.display()
disp.init()

disp_buf1 = lv.disp_buf_t()
buf1_1 = bytearray(320*10)
lv.disp_buf_init(disp_buf1,buf1_1, None, len(buf1_1)//4)
disp_drv = lv.disp_drv_t()
lv.disp_drv_init(disp_drv)
disp_drv.buffer = disp_buf1
disp_drv.flush_cb = disp.flush
lv_disp = lv.disp_drv_register(disp_drv)

indev_drv = lv.indev_drv_t()
lv.indev_drv_init(indev_drv) 
indev_drv.type = lv.INDEV_TYPE.POINTER
indev_drv.read_cb = tp.read
lv.indev_drv_register(indev_drv)

class DemoPage():
	def __init__(self):
		self.PrevPage = lv.disp_get_scr_act(lv_disp)
		self.CurPage = lv.obj()
		self.CurPage.set_size(320, 240)
		lv.disp_load_scr(self.CurPage)
		self.BtnValue = 1000
		self.Plus1Btn = lv.btn(self.CurPage)
		self.Plus1Btn.set_pos(10, 25)
		self.Plus1Btn.set_size(50, 40)
		self.Plus1Btn.set_event_cb(self.Plus1EventCB)
		self.Plus1Label = lv.label(self.Plus1Btn)
		self.Plus1Label.set_text("+1")
		self.Plus100Btn = lv.btn(self.CurPage)
		self.Plus100Btn.set_pos(10, 75)
		self.Plus100Btn.set_size(50, 40)
		self.Plus100Btn.set_event_cb(self.Plus100EventCB)
		self.Plus100Label = lv.label(self.Plus100Btn)
		self.Plus100Label.set_text("+100")
		self.Minus1Btn = lv.btn(self.CurPage)
		self.Minus1Btn.set_pos(10, 125)
		self.Minus1Btn.set_size(50, 40)
		self.Minus1Btn.set_event_cb(self.Minus1EventCB)
		self.Minus1Label = lv.label(self.Minus1Btn)
		self.Minus1Label.set_text("-1")
		self.Minus100Btn = lv.btn(self.CurPage)
		self.Minus100Btn.set_pos(10, 175)
		self.Minus100Btn.set_size(50, 40)
		self.Minus100Btn.set_event_cb(self.Minus100EventCB)
		self.Minus100Label = lv.label(self.Minus100Btn)
		self.Minus100Label.set_text("-100")
		self.ValueLable = lv.label(self.CurPage)
		self.ValueLable.set_text("Value:")
		self.ValueLable.set_align(lv.ALIGN.CENTER)
		self.ValueLable.set_pos(140, 110)
		self.ValueText = lv.ta(self.CurPage)
		self.ValueText.set_pos(190,100)
		self.ValueText.set_size(100,40)
		self.ValueText.set_text(str(self.BtnValue))		
	def Plus1EventCB(self, obj=None, event=-1):
		if event == lv.EVENT.CLICKED:
			print("+1 click")
			self.BtnValue = self.BtnValue + 1
			self.ValueText.set_text(str(self.BtnValue))		
	def Plus100EventCB(self, obj=None, event=-1):
		if event == lv.EVENT.CLICKED:
			print("+100 click")
			self.BtnValue = self.BtnValue + 100
			self.ValueText.set_text(str(self.BtnValue))		
	def Minus1EventCB(self, obj=None, event=-1):
		if event == lv.EVENT.CLICKED:
			print("-1 click")
			self.BtnValue = self.BtnValue - 1
			self.ValueText.set_text(str(self.BtnValue))		
	def Minus100EventCB(self, obj=None, event=-1):
		if event == lv.EVENT.CLICKED:
			print("-100 click")
			self.BtnValue = self.BtnValue - 100
			self.ValueText.set_text(str(self.BtnValue))		

if tp.iscalibrated() == False:
	tp.calibrate()

DemoPage()
while(True):
	lv.task_handler()


