from pyb import SPI
spi0=SPI(0, SPI.MASTER, baudrate=5000, bits=32)
data = spi0.read(64)
