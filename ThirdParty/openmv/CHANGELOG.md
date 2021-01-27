# Change Log
## [3.9.1](https://github.com/openmv/openmv/releases/tag/v3.9.1) (2021-01-22)
* Update ulab to 2.1.3
* Add Portenta LoRa library.
* Add set_framerate to sensor module.
* Enable exFAT for Portenta.
* Update HM01B0 sensor driver.
* Improve low-power modes on QSPI boards.
* Reduced firmware image size.
* Faster firmware build.

## [3.9.0](https://github.com/openmv/openmv/releases/tag/v3.9.0) (2021-01-13)
* Fix H7 timer bug in new HAL. 
* Fix pyb_spi bug from upstream.
* Fix OV5640 PCLK calculation.
* Add support for the MicroPython nRF port.
* Add initial NANO 33 BLE sense board support.
* Add support for OV5640 auto-focus feature.
* Add support for the new MLX90641 FIR sensor.
* Add the MLX90621 official driver library.
* Update tinyusb submodule to 0.7.0
* Update cambus code to work with all supported FIR sensors.
* Update cambus to support auto-detection of FIR sensors.
* Update read_ir() to support hmirror, vflip and tranpose.
* Update draw_ir() to use the new drawing pipeline with bicubic scaling.
* Add ImageIO type to support memory stream I/O.
* Update example scripts.
* Add new examples for NANO 33.

## [3.8.0](https://github.com/openmv/openmv/releases/tag/v3.8.0) (2020-12-06)
* Update F4 HAL V1.7.1 -> V1.7.10
* Update F7 HAL V1.2.2 -> V1.2.8
* Update H7 HAL V1.6.0 -> V1.9.0
* Update MicroPython 1.12 -> 1.13
* Fix FIR I2C bus arbitration lost error.
* Fix disabled OSCs/PLLs after exiting stop mode.
* Fix DRAM retention in stop mode.
* Fix stop mode voltage scaling for H7 Rev V.
* Fix OV5640 and OV2640 clocks on H7 Rev Y.
* Fix imlib build dependencies.
* Fix fatal_error possibly using uninitialized storage.
* Fix build errors if no DCMI GPIOs are defined.
* Fix RGB565 inversion from camera.
* Fix copy_to_fb to update JPEG fb after loading/creating images.
* Fix PLL1 frequency for revision Y devices.
* Fix unit-test failing on disabled functions.
* Add FT5X06 touch screen lcd support.
* Add HDMI CEC and HDMI support via the TFP410.
* Add DMA2D support to some drawing functions.
* Add power supply per-board configuration.
* Add new image scaling pipeline.
* Add ExtInt wake-up example.
* Add support for using images to draw on FB.
* Add CAN function to calculate bit timing from baudrate.
* Update SDRAM test and make it cache-aware.
* Update cpufreq module to support H7 REV X/Y and V frequencies.
* Update from using frozen dir to per-board frozen manifest file.
* Portenta: Fix and update PLL settings.
* Portenta: Fix SDRAM timing config.
* Portenta: Add Audio module with PCM support.
* Portenta: Add Tensorflow micro speech module.
* Portenta: Fix Ethernet/SDRAM bug.
* Portenta: Add Portenta audio example scripts.
* Portenta: Add Ethernet support and example scripts.

## [3.7.0](https://github.com/openmv/openmv/releases/tag/v3.7.0) (2020-11-23)
* Fix imlib build dependencies.
* Fix fatal_error bug using uninitialized storage.
* Fix build errors if no DCMI GPIOs are defined.
* Fix RGB565 inversion from camera
* Fix copy_to_fb to update JPEG fb after loading/creating images.
* Fix PLL1 frequency for revision Y devices.
* Add FT5X06 touch screen lcd support.
* Add HDMI support via the TFP410
* Use DMA2D to accelerate some drawing functions.
* Update SDRAM test and make it cache-aware.
* Add power supply per board configuration.
* Add new image scaling pipeline
* Add ExtInt wake-up example.
* Support flushing images without initializing the sensor.
* PORTENTA: Fix and update PLL settings.
* PORTENTA: Fix SDRAM timing config.
* PORTENTA: Add Audio module with PCM support.
* PORTENTA: Add Tensorflow micro speech module.
* PORTENTA: Fix Ethernet/SDRAM bug.
* PORTENTA: Add Portenta audio example scripts.

## [3.6.9](https://github.com/openmv/openmv/releases/tag/v3.6.9) (2020-10-12)
* Enable LWIP and CYW4343.
* Fix major issues with Portenta build.
* Fix issues with QSPI and MPU mode.

## [3.6.8](https://github.com/openmv/openmv/releases/tag/v3.6.8) (2020-9-28)
* Add support for boards without image sensors.
* Enable TensorFlow CMSIS-NN kernels.
* Add UART and CAN support to RPC library.
* Update example scripts.
* Allow build-time configuration of clock sources, PLLs, sensors and modules.
* Improve Arduino's Portenta board support.
* Fix test jig bug.
* Fix extra PLLs initialization in system config.
* Fix FLIR alt configuration.
* Fix sensor reset with the regulator turned off.
* Fix uninitialized sensor timer struct field bug.
* Fix JPEG image loading.
* Fix broken debug build.
* Fix FSYNC pin definition.
* Fix DAC timed write bug on H7.
* Fix WINC1500 open security bug.

## [3.6.7](https://github.com/openmv/openmv/releases/tag/v3.6.7) (2020-7-21)
* Fix framebuffer bug introduced in 3.6.5.

## [3.6.6](https://github.com/openmv/openmv/releases/tag/v3.6.6) (2020-7-18)
* Fix broken sensor_init0.
* Add face recognition scripts.

## [3.6.5](https://github.com/openmv/openmv/releases/tag/v3.6.5) (2020-7-17)
* Fix minor FPS counter bug.
* Update ulab to the latest.
* Improved Portenta board support.
* Fix ST CUBE-AI build.
* Update tensorflow library to the latest.
* Remove legacy CMSIS-NN code and examples.
* Code refactoring and cleanup.

## [3.6.4](https://github.com/openmv/openmv/releases/tag/v3.6.4) (2020-6-03)
* Fix OV5640 imaging modes.
* Fix floating point scaling in TensorFlow code.
* Fix software JPEG encode of 1-bpp images.
* Fix grayscale SW JPEG compression for YCbCr colorspace.
* Fix flipped bayer images on OV7725 and OV7690.
* Improved OV5640 FPS.
* Improved sensor driver for higher FPS.
* Harden sensor driver code.
* Support debayering for get_pixel in bayer mode.
* Update cascade converter to work with Python3.
* Add rtsp support.

## [3.6.3](https://github.com/openmv/openmv/releases/tag/v3.6.3) (2020-5-16)
* Update TF to support uint8/int8/float32
* Re-enable TF on OMV3/F7.
* Improved imlib lens correction.
* Fix rotation correction bug.
* Update MLX90640 driver.
* Improved FIR sensors support.
* Support higher framerate on OV5640.
* Improved drawing and blending functions.
* Fix Bayer to YCBCR edge bug.

## [3.6.2](https://github.com/openmv/openmv/releases/tag/v3.6.2) (2020-5-04)
* Optimized Bayer to RGB565/YCBCR.
* Optimized datamatrix ops and binary ops.
* Improved WiFi throughput.
* Add raw data RD/WR access to the image object.
* Support draw image with custom palettes.
* Add new OpenMV RPC Interface Library.
* Fix H7 SPI DMA bugs.
* Fix H7 SPI and I2C DMA Deinit.
* Fix and update draw_image.
* Updated tensorflow library to support int8
* Fix H7 FDCAN bugs.
* Fix bug in filters corrupting memory if y size less than k_size.

## [3.6.1](https://github.com/openmv/openmv/releases/tag/v3.6.1) (2020-3-30)
* Add support for OV7690 and HM01B0 sensors.
* Add support for Portenta-H747.
* Optimized image filters, lens correction and find_circles.
* Add BGR argument to lcd init (fixes an issue with some LCDs).
* Move clock configuration to board config file.
* Fix hardfault on disable D cache. 
* Fix Tensorflow stack overflow issue.
* Fix load_to_fb bug.

## [3.6](https://github.com/openmv/openmv/releases/tag/v3.6) (2020-2-7)
* Fix H7 DAC timed write bug.
* Fix self-tests bug.
* Fix H7 I2C timings.
* Fix TIM4 reserved bug.
* Update to MicroPython v1.12
* Support for 32MBs QSPI flash.
* Bootloader QSPI flash support.
* OMV4+ UVC support.
* OV5640 sensor driver.
* Optimized LSD, AprilTags, QRCode and JPEG.
* Updated py_tf tensorflow docs.

## [3.5.2](https://github.com/openmv/openmv/releases/tag/v3.5.2) (2019-12-17)
* Fix H7 timer bug.
* Update timer test scripts.
* Enable DBGMCU in sleep modes.
* Fix MQTT module to work with SSL sockets.

## [3.5.1](https://github.com/openmv/openmv/releases/tag/v3.5.1) (2019-12-06)
* Update ulab submodule.
* Update Tensorflow library.
* Fix sepconv3 bug.
* Fix debug build errors.

## [3.5.0](https://github.com/openmv/openmv/releases/tag/v3.5.0) (2019-11-04)
* Update CMSIS to v5.4.0
* Update H7 HAL to v1.5
* Update MicroPython to 1.11.
* Update WINC1500 firmware to v19.6.1.
* Update WINC1500 host driver to v19.3.0.
* Add STM32Cube.AI support.
* Add TensorFlow Lite for microcontrollers.
* Add built-in person detector with TF Lite.
* Add ulab and openrv libraries.
* Add support for 32-bit SDRAM @100MHz.
* Add Arduino UART example.
* Add new ADC example for internal channels.
* Add new HTTPs client examples.
* Fix fb_alloc bug introduced in v3.5.0-beta.2.
* Fix ADC driver to work with new H7 HAL.
* Fix BMP bug when reading 24-bit images.
* Fix Lepton Hardfault when setting VGA/RGB565.
* Fix SPI WFI bug on F7.
* Fix cpufreq H7 frequencies.
* Fix Makefile order dependency issues.
* Fix VSCALE0 low-power mode.
* Enable mod USSL with MBEDTLS.
* Enable QSPI internal storage for OpenMV-4R2.
* Enable VSCALE0 for rev V devices.
* All the modules in scripts/libraries are now frozen.

## [3.5.0-beta.3](https://github.com/openmv/openmv/releases/tag/v3.5.0-beta.3) (2019-10-25)
* Update WINC1500 to firmware v19.6.1 and host driver v19.3.0.
* Add STM32Cube.AI support.
* Fix fb_alloc bug introduced in v3.5.0-beta.2.
* Enable QSPI internal storage for OpenMV-4R2.
* Switch to VSCALE1 before entering low-power mode.
* Add support for TensorFlow Lite for Microcontrollers.
* Enable mod USSL with MBEDTLS.
* Update HTTP/S client examples.
* Fix Makefile order dependency issue causing non-parallel builds to fail.

## [3.5.0-beta.2](https://github.com/openmv/openmv/releases/tag/v3.5.0-beta.2) (2019-10-12)
* Update to CMSIS v5.4.0
* Update to H7 HAL v1.5
* Update ADC driver to work with new H7 HAL.
* Enable VSCALE0 for rev V devices.
* Enable PLL3 for ADC and SPI123v and use PLL2 for FMC (outputs 200MHz).
* Add support for 32-bit SDRAM @100MHz.
* Fix BMP bug when reading 24-bit images.
* Update ADC examples
* Add new ADC example for internal channels.
* Add Arduino UART example.
* Update Arduino SPI example to use callbacks.
* Add PWM channel 3 and servo 3 to pwm and servo examples.
* Fix Lepton Hardfault when setting VGA/RGB565.

## [3.5.0-beta.1](https://github.com/openmv/openmv/releases/tag/v3.5.0-beta.1) (2019-09-30)
* Update to MicroPython 1.11.
* Update examples.
* Fix SPI WFI bug on F7.

## [3.4.3](https://github.com/openmv/openmv/releases/tag/v3.4.3) (2019-09-27)
* Fix delay when JPEG encoding overflows (affects H7).

## [3.4.2](https://github.com/openmv/openmv/releases/tag/v3.4.2) (2019-09-16)
* Fix H7 RTC bugs.
* Fix binary ops bug.
* Fix H7 deepsleep mode.
* Fix JPEG mode bugs.
* Fix H7 DMA bug.
* Fix LBP ROI bug.
* Update OV2640 driver.
* Add support for OV5640.
* Add new SDRAM board support.
* Add new libraries and examples.
* Add FB alloc statistics, enable with (FB_ALLOC_STATS=1).
* Add support for H7 FDCAN.
* Enable btree module.
* AprilTag: support flipped/mirrored images.
* AprilTag: support high resolution images.
* WINC1500: Add netinfo function.
* WINC1500: Support static IPs.
* WINC1500: Fix timeout issues with WINC wrapper.
* WINC1500: Fix accept() hardfault on unbound sockets.

## [3.4.1](https://github.com/openmv/openmv/releases/tag/v3.4.1) (2019-05-02)
* This patch release fixes an issue with Lepton clock.

## [3.4](https://github.com/openmv/openmv/releases/tag/v3.4) (2019-04-30)
* Update NN models.
* Add SSD1306 OLED driver.
* Add more Python examples.
* Upgrade H7 clock to 480MHz.
* Fix ctrl-c on REPL UART.
* Fix WINC crashing on select/poll calls.
* Fix SCCB/I2C timing for F7&H7 (set to ~100KHz).
* Fix H7 SD clock.
* Fix broken OpenMV-2 firmware.
* Fix binary function in RGB565 mode.
* Fix frozen modules build.

## [3.3.1](https://github.com/openmv/openmv/releases/tag/v3.3.1) (2019-03-23)
* Fix NN enum size bug.
* Fix H7 JPEG encoder bug.

## [3.3](https://github.com/openmv/openmv/releases/tag/v3.3) (2019-03-18)
* Update FatFS to FF13C.
* Update FLIR and MT9V034 drivers.
* Add new libraries and examples.
* Add masking functions.
* Add configurable color palettes.
* Add FLIR measurement mode (AGC disabled).
* Add py TV module support.
* Fix H7 ADC bug.
* Fix JPEG MCU boundaries.
* Fix exFAT bug.
* Fix fb_alloc_mark bug. 

## [3.2](https://github.com/openmv/openmv/releases/tag/v3.2) (2018-11-04)
* Fix column buffer bug in CMSIS-NN library.
* Fix H7 SPI clock source.
* Fix bug in LBP.
* Fix WINC1500 initialization timeout bug.
* Fix REPL on UART bug.
* Use DMA in FLIR/Lepton driver.
* Update NN documentation.
* Update to CMSIS NN 5.4.0
* Faster UVC streaming.
* Enable exFAT for OpenMV F7 and H7.

## [3.1](https://github.com/openmv/openmv/releases/tag/v3.1) (2018-10-02)
* Fix/Re-enable CAN.
* Fix WINC recv buffer bug.
* Fix SPI timeout in slave mode.
* Improved FLIR drivers.
* Add sensor shutdown function.
* Add selective search
* Add support for WiFi programming.
* Add new UVC firmware.
* Add support for the new MLX sensors.

## [3.0](https://github.com/openmv/openmv/releases/tag/v3.0) (2018-06-29)
* Fix SPI driver bug.
* Fix pendsv hardfault bug.
* Fix WINC driver bug.
* Fix collections list_pop_front bug.
* Re-enable OMV2 build.
* Update to MicroPython 1.9.4.
* Add support for loading NN models.
* Add NN quantization and converter scripts and example models.
* Add support for running Haar on RGB images.
* Add support for running keypoints on RGB images.

## [2.9](https://github.com/openmv/openmv/releases/tag/v2.9) (2018-05-06)
* Fix BAYER boundary issue.
* Re-enable SD DMA transfers.
* Fix bug in non-DMA SD transfers.
* Add FLIR Lepton1/2/3 support.

## [2.8](https://github.com/openmv/openmv/releases/tag/v2.8) (2018-04-23)
* Improved H7 support.
* Enable text scaling.
* Fix Image.save bug.
* Make imlib more configurable.
* Update MicroPython.
* Add CMSIS NN example.
* Add new example scripts.
* Improved JPEG quality (for M7 and H7).
* Add color thresholding support to get_histogram/stats.

## [2.7](https://github.com/openmv/openmv/releases/tag/v2.7) (2018-01-24)
* Add LeNet NN.
* Add shadow removal.
* Implement low-power modes.
* Update WINC firmware.
* Update gain, exposure and white balance controls.
* Add MQTT library and example.
* Support BAYER get_pixel().
* Fix getaddrinfo bug.
* Fix find template bug.

## [2.6](https://github.com/openmv/openmv/releases/tag/v2.6) (2017-11-04)
* Update to MicroPython 1.9.2
* Support saving bayer (raw) images.
* Add perspective rotation correction code.
* Fix blob density.
* Fix color VGA image save.
* Remove invalid resolutions. 

## [2.5](https://github.com/openmv/openmv/releases/tag/v2.5) (2017-08-10)
* Fix UART timeout when using slow baudrate.
* Enable RTC.
* Remove openmv.inf and update Readme.
* Support recording and viewing raw videos.
* Add linear regression.
* Add find_rectangles and find_circles.
* Improve find_lines merging.
* Fix bug in ORB matching descriptor loaded from file.
* Support new OpenCV Haar format.
* Fix bug in Haar cascades loading.
* Add initial LeNet port.
* Add unit-tests.
* Fix uninitialized FB enabled bug.
* Fix Servo(3).
* Fix MJPEG/GIF BAYER support.

## [2.4.1](https://github.com/openmv/openmv/releases/tag/v2.4.1) (2017-06-04)
* Upstream Kanji fix.
* Upstream MP SCSI fix.
* Fix binary ops names.

## [2.4](https://github.com/openmv/openmv/releases/tag/v2.4) (2017-05-30)
* Implement faster line detection algorithm.
* Support line segments detection.
* Support higher FPS on OpenMV 2 and 3.
* Add data matrix support.
* Add more small resolutions.
* Enable UART1 on OpenMV3/M7
* Enable VSYNC output on IO pin.
* Fix QR Code bug.
* Fix UDP recvfrom bug.
* Minor fixes, typos and docs updates.

## [2.3](https://github.com/openmv/openmv/releases/tag/v2.3) (2017-03-26)
* Support WiFi Access Point mode.
* New BAYER/RAW pixel format.
* Support RGB VGA frames.
* 1D barcode support using (zbar).

## [2.2](https://github.com/openmv/openmv/releases/tag/v2.2) (2017-02-28)
* Add Apriltags support
* Fix OMV3 bootloader LED pins
* Enable CAN
* Enable extra MP modules (json, re, zlib, hashlib, binascii, random)
* Add Pixy emulation.
* QR Code bug fixes

## [2.1](https://github.com/openmv/openmv/releases/tag/v2.1) (2017-01-21)
* New keypoints descriptor (ORB).
* QR decoding via quirc library (https://github.com/dlbeer/quirc)
* Support load to FB directly in copy_to_fb.
* Export lens shading function.
* Add AGAST corner detector.
* Implement set_gain/exposure/whitebalance functions.
* Various optimizations and speedups
* Fix uSD cache issues on M7.
* Fix broken ADC on M4 and M7.
* Fix image compress and compressed.
* Fix ff_wrapper bug.
* Fix OMV3 LEDs pinout

## [2.0](https://github.com/openmv/openmv/releases/tag/v2.0) (2016-11-04)
Firmware:
* WiFi driver fixes.

Image processing:
* Add HoG (not used yet).
* Add lens correction function.
* Add clear image for quick testing.
* Fix template ROI.
* Switch to FAST-12.
* Misc fixes to image library.

## [1.9](https://github.com/openmv/openmv/releases/tag/v1.9) (2016-09-20)
Firmware:
* Initialize RNG when calling randint.

Image processing:
* Fix and update Kmeans code.
* Add ellipse masking function.
* Add face recognition code and example script.
* Add Hough Transform code and example script.
* Add Canny edge code and example script.
* Add Gaussian function for quick testing.

## [1.8](https://github.com/openmv/openmv/releases/tag/v1.8) (2016-08-31)
Firmware:
* Mainly WiFi driver fixes, more stable streaming, timeouts and better error handling.
* Fixed FPS slow down in dark images (max FPS reduction is 30FPS)

## [1.7](https://github.com/openmv/openmv/releases/tag/v1.7) (2016-08-25)
Firmware:
* Update CMSIS, DSP lib and HAL.
* Adaptive JPEG quality based on JPEG frame size.
* Improved self-tests on OV7725.
* New CPU frequency scaling Python module.
* Allow setting MLX refresh rate and ADC resolution.
* Use a dedicated JPEG buffer (improves IDE FPS).

## [1.6](https://github.com/openmv/openmv/releases/tag/v1.6) (2016-07-27)
IDE:
* Add checkbox to disable the framebuffer update

Firmware:
* Set FB JPEG quality/subsampling based on frame size.

Image processing:
* Implement windowing.
* Implement horizontal and vertical binning.
* Implement optical flow with phase correlation.
* Implement copy image to framebuffer for testing.
* Allow ROIs and step in template matching function.
* Implemented diamond search for fast template matching.
* Fix bug in integral_image_sq and lookup.
* Add new smaller resolutions
* Improved/fixed JPEG code

## [1.5](https://github.com/openmv/openmv/releases/tag/v1.5) (2016-06-01)
IDE:
* Fix pinout reference image.
* Fix reset on bootloader (reset cam just before the bootloader runs).
* Add an option to erase flash filesystem sectors in bootloader dialog.
* Show color statistics in a message dialog.

Firmware:
* Update to MicroPython v1.8
* Change MLX ADC resolution to 18 bits.
* Fixed GC collect bug (.bss and .data were not scanned, fixed in MP update).
* Generate a combined (bootloader + app) dfu and binary images
* Rename firmware images:
 - bootloader.xxx (CDC bootloader images)
 - firmware.xx (main application firmware images)
 - openmv.xx (combined bootloader+firmware images)

Image processing:
* Allow image line-by-line pre-processing from Python callbacks.

## [1.4](https://github.com/openmv/openmv/releases/tag/v1.4) (2016-05-02)
IDE:
* Fix text editor undo bug.
* New bootloader dialog.
* Fix and update example scripts.
* Fix preferences dialog.
* Remove refresh button.

Firmware:
* Fixed file wrapper initialization bug.
* New CDC-based bootloader (works on Linux, Windows and OSX)
* Implement new sensor functions (disable AGC and AEC)
* Fix WINC bug overriding sent data.

Image processing:
* Color codes support.
* New color blob detector.

## [1.3](https://github.com/openmv/openmv/releases/tag/v1.3) (2016-04-07)
IDE:
* Implement the IDE copy color function.
* Update examples menu using categories.
* Fix conflict with PyInstaller scripts.

Firmware:
* Add initial WiFi (WINC1500) support.
* Update WINC1500 driver and firmware to 19.4.4
* Support WINC1500 firmware update from uSD fw image.
* Improved MLX (FIR) temperature scaling and drawing.
* Add WiFi examples (mjpeg streamer, NTP, scan, connect and firmware update)

Image processing:
* Implement AWB/HMirror/VFlip.
* Implement mean, median and mode filters.

## [1.2](https://github.com/openmv/openmv/releases/tag/v1.2) (2016-03-19)
IDE:
* About dialog, license and credits.
* Pin-out image for quick reference.
* Check for updates on startup.
* Support older firmware versions.
* Retry a few times when connecting.
* Enable/Disable framebuffer JPEG compression.

Firmware:
* Support the newer OV7725 sensor.
* Add snapshot timeout to avoid locking the cam.
* Fix PWM/Servos timer, channels and pin-mappings. 
* Add OpenMV boards configuration files in omv/boards.
* Support the new MLX90621 sensor and add proper rainbow scaling.
* Better script handling, and soft reset support.
* JPEG-compress the framebuffer to lower the bandwidth and fake double-buffering.
* YUV to Grayscale conversion on the fly.
* Add sanity checks and more meaningful error messages.
* Allocate FatFS LFN buffer on stack (frees 255 heap bytes).
* Move the FatFS and MSC buffers to main RAM (saves heap and allows DMA access).
* Use DMA for SDIO transfers.
* Remove framebuffer mutex (IDE reads images before snapshots).
* Define pin aliases (P0..P8)/
* Move LCD to built-in module.

Image processing:
* Improved iris detection.
* Edge detection, generic convolution, motion detection and GIF support.
* JPEG compressor optimizations (70ms @QVGA 320x240) faster BinDCT and 2x2 subsampling.
* Proper JPEG headers for Grayscale images.
* Bug fixes in old integral image code and a new integral image using a moving window.
* Set the number of pixels counted in each blob in imlib_count_blobs.
* Simplify the image descriptor APIs, use a generic image.load/save/match_descriptor functions.
* Add HQVGA resolution, and special digital effects support.
* Support higher Grayscale resolutions (up to QVGA) for most algorithms.
* Image processing functions accept paths to images on uSD.

## [1.1](https://github.com/openmv/openmv/releases/tag/v1.1) (2015-08-15)
* Rollback to gtksourceview
* Use MP peripherals
* Add ABI version and check it in the IDE
* Add common cascades to the flash
* Fix changing pixformat bug
* Fix sensor reset
* gc/xalloc race bug
* Fix sensor clock
* Update to MP 1.4.4
* Add udev rules help and check for udev file
* Update USB PID:VID
* Update inf file
* Generate Linux/Windows packages
* Catch and print syntax errors
* Add colorbar mode function
* Optimize the IDE (revert to numpy, use timeout_add etc..)
* Remove obselete #define from mpconfigboard.h
* Write colorbar test
* Fix silkscreen
* Rename Eagle files
* Move misc image functions to image module
* Delay sensor init after USB storage to log errors to file
* Implement get/set pixel
* Fix push/pop scope (re-init mp before running scripts)
* Update examples
* Fix main script FS template in main.c
* Remove global misc functions
* Remove lib folder
* Fix draw_string
* Disable built-in DFU on Windows

## [1.0.3-beta](https://github.com/openmv/openmv/releases/tag/v1.0.3-beta) (2014-11-15)
* Binary packaged using py2exe
* Mixed 32/64 bit Windows installer
* Fix USB issue on Windows 7 64-bit
* Enable color-lookup (was disabled in binaries)

## [1.0.2-beta](https://github.com/openmv/openmv/releases/tag/v1.0.1-beta) (2014-11-11)
* Fixes USB issues on Windows.
* New MSI package for Windows users
* Moved all user data are stored in home directory.

## [1.0.1-beta](https://github.com/openmv/openmv/releases/tag/v1.0.1-beta) (2014-11-2)
* Minor fixes for compatibility with Windows.

## [1.0.0-beta](https://github.com/openmv/openmv/releases/tag/v1.0.0-beta) (2014-10-31)
* First release.
