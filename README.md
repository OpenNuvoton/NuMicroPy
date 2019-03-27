# NuMicroPy
NuMicroPy is Nuvoton microcontroller porting for MicroPython. MicroPython is a lean and efficient implementation of the Python 3 programming language that includes a small subset of the Python standard library and is optimised to run on microcontrollers and in constrained environments. See [MicroPython](http://micropython.org/)

----
## Major components in this repository
- M480BSP/ -- NuMicro M480 series BSP
- build/ -- Prebuilt frimware
- patch/ -- BSP/MicroPython patch files
- M48x/ -- M480 series porting of MicroPython
- micropython/ -- MicroPython official release(v1.10)

----
## Supported target
Board            |MCU      |ROM size  |RAM size
:----------------|---------|----------|-------
NuMaker-PFM-M487 |M487     |362KB     |77KB

----
## How to start NuMicroPy
1. Download and install [Nu-Link Command Tool](https://www.nuvoton.com/hq/support/tool-and-software/software/programmer/?__locale=en)
2. Hardware setup steps  
a. Turn on ICE function switch pin 1,2,3 and 4  
![NuMaker-PFM-M487](https://i.imgur.com/tFvodDh.jpg)  
b. Connect USB ICE and USB1.1 to PC  
c. Setup your terminal program  
![TeraTerm_setup1](https://i.imgur.com/w598b7y.jpg)  
![TeraTerm_setup2](https://i.imgur.com/hkBwaJv.jpg) 
3. Burn firmware  
Nu-Link-Me exported a MBED disk, just Copy and Paste prebuilt firmware.bin into MBED disk.  
![CopyPasteFirmware](https://i.imgur.com/XcHo5fP.jpg)
4. Python code update steps  
a. Connected USB1.1 to PC  
b. Press the SW2 and RESET button together. Firmware will export a PYBFLASH disk.  
![PYBFLASH disk](https://i.imgur.com/111q3XP.jpg)  
c. Update your python code to boot.py or main.py
![Main Code](https://i.imgur.com/fRwYR5x.jpg)  
d. Press the RESET button.  
![Execute Result](https://i.imgur.com/ZExBjT0.jpg)  


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
cd ../M48x  
make V=1
```

----
