# NuMicroPy
NuMicroPy is Nuvoton microcontroller porting for MicroPython. MicroPython is a lean and efficient implementation of the Python 3 programming language that includes a small subset of the Python standard library and is optimised to run on microcontrollers and in constrained environments. See [MicroPython](http://micropython.org/)

----
## Major components in this repository
- M480BSP/ -- NuMicro M480 series BSP
- M261BSP/ -- NuMicro M261 series BSP
- build/ -- Prebuilt frimware
- patch/ -- BSP/MicroPython patch files
- M48x/ -- M480 series porting of MicroPython
- M26x/ -- M261 series porting of MicroPython
- micropython/ -- MicroPython official release(v1.10)
- ThirdParty/ -- Third party library

----
## Supported target
Board            |MCU      |ROM size            |RAM size
:----------------|---------|--------------------|------------------
NuMaker-PFM-M487 |M487     |364KB/648KB(W/lvgl) |75KB/79KB(W/lvgl)
NuMaker-IOT-M487 |M487     |322KB               |46KB
NuMaker-M263KI   |M263     |266KB               |35KB

----
## How to start NuMicroPy
1. Download and install [Nu-Link Command Tool](https://www.nuvoton.com/hq/support/tool-and-software/software/programmer/?__locale=en)
2. Hardware setup steps  
a. Turn on ICE function switch pin 1,2,3 and 4  
![NuMaker-PFM-M487](https://i.imgur.com/tFvodDh.jpg)  
b. Connect USB ICE and USB1.1 to PC  
c. Setup your terminal program  
![TeraTerm_setup1](https://imgur.com/oKbxDJ2.jpg)  
![TeraTerm_setup2](https://imgur.com/tuYp3xh.jpg) 
3. Burn firmware  
Nu-Link-Me exported a "NuMicro MCU" disk, just Copy and Paste prebuilt firmware.bin into "NuMicro MCU" disk.  
![CopyPasteFirmware](https://imgur.com/RcxvyHH.jpg)
4. Python code update steps  
a. Connected USB1.1 to PC  
b. [NuMaker-PFM-M487 and NuMaker-IOT-M487]: Press the SW2 and RESET button together. [NuMaker-M263KI]: Press the RESET button. Firmware will export a PYBFLASH disk.
![PYBFLASH disk](https://imgur.com/Q7mp628.jpg)  
c. Update your python code to boot.py or main.py
![Main Code](https://imgur.com/WGUv4MM.jpg)  
d. Press the RESET button.  
![Execute Result](https://imgur.com/BVns53g.jpg)  


----
## How to build firmware
The development of MicroPython firmware is in Unix-like environment. The description as below will be using Ubuntu 16.04.
1. Packages Requirement  
The following packages will need to be installed before you can compile and run MicroPython
	* build-essential
	* libreadline-dev
	* libffi-dev
	* git
	* pkg-config

    To install these packages using the following command.
```
sudo apt-get install build-essential libreadline-dev libffi-dev git pkg-config
```
2. Install GNU Arm Toolchain  
Download GNU Arm toolchain linux 64-bit version 7-2018-q2 update from [Arm Developer](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). Next use the tar command to extract the file to your favor directory (ex. /usr/local)  
```
mv gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2 /usr/local/  
cd /usr/local  
tar -xjvf gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2  
```   
Now, modify your PATH environment variable to access the bin directory of toolchain

3. Build firmware  
To build MicroPython firmware for M487, use the following command.
```
git clone --recursive https://github.com/OpenNuvoton/NuMicroPy.git  
cd patch  
./run_patch.sh
```
For M480 series
```  
cd ../M48x  
#For NuMaker-PFM-M487 board
make V=1
#For NuMaker-IOT-M487 board
make BOARD=NuMaker-IOT-M487 V=1
```
For M261 series
```
cd ../M26x
#For NuMaker-M263KI board
make V=1
```

----
## How to run [LittlevGL](https://littlevgl.com/)
![M487Advance](https://imgur.com/xhgy5ZN.jpg)
1. Hardware requirement: NuMaker-PFM-M487 + M487 Advance Ver 4.0  
2. Burn firmware.bin (build/NuMaker-PFM-M487/WithLittlevGL/) to APROM and firmware_spim.bin (build/NuMaker-PFM-M487/WithLittlevGL) to SPI flash  
a. Execute [NuMicro ICP programming tool](https://www.nuvoton.com/hq/support/tool-and-software/software/programmer/?__locale=en) and connect to target chip(M480 Series)  
![ICP_Connect](https://imgur.com/nUYJy1M.jpg)  
b. Check SPIM multi-function pin select to PC2/PC3/PC1/PC0  
![ICP_SPIM_PIN](https://imgur.com/eTlNyF0.jpg)  
c. Programming config setting first  
![ICP_PROG_CONF](https://imgur.com/VCyxxxI.jpg)  
e. Programming APROM(firmware.bin) and SPI flash(firmware_spim.bin)  
![ICP_PROG_CODE](https://imgur.com/QJYbj7k.jpg)  
f. ICP tool disconnect target chip and press NuMaker-PFM-M487 RESET button  
![ICP_Disconnect](https://imgur.com/U8tTEEi.jpg)  
![SPIM_TERM](https://imgur.com/C2PEiBH.jpg)  
3. Please follow "How to start NuMicroPy" section 4. Copy example code(M48x/example/LittlevGL.py) to main.py  
![LVGL_EXAM](https://imgur.com/3BFyKx4.jpg)  
![LVGL_TERM](https://imgur.com/8RMKF1F.jpg)  

----
