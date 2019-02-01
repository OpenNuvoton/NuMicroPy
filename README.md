# NuMicroPy
NuMicroPy is Nuvoton microcontroller porting for MicroPython. MicroPython is a lean and efficient implementation of the Python 3 programming language that includes a small subset of the Python standard library and is optimised to run on microcontrollers and in constrained environments. See [MicroPython](http://micropython.org/)

----
## Major components in this repository
- M480BSP/ -- NuMicro M480 series BSP
- patch/ -- BSP/MicroPython patch files
- M48x/ -- M480 series porting of MicroPython
- micropython/ -- MicroPython official release(v1.10)

----
## Supported target
Board            |CPU      |ROM size  |RAM size
:----------------|---------|----------|-------
NuMaker-PFM-M487 |M487     |315kB     |76kB

---
## How to build firmware
1. git clone --recursive https://github.com/OpenNuvoton/NuMicroPy.git
2. cd patch
3. ./run_patch.sh
4. cd ../M48x
5. make

----
