#!/bin/sh
cp -f retarget.c ../M480BSP/Library/StdDriver/src/retarget.c
cp -f pdma.c ../M480BSP/Library/StdDriver/src/pdma.c
cp -f fixed.h ../M480BSP/ThirdParty/LibMAD/inc/fixed.h
cp -f layer3.c ../M480BSP/ThirdParty/LibMAD/src/layer3.c
cp -f wblib.h ../N9H26_emWin_NonOS/BSP/Driver/Include/wblib.h
cp -f sdGlue.c ../N9H26_emWin_NonOS/BSP/Driver/Source/SIC/sdGlue.c
cp -f fmi.h ../N9H26_emWin_NonOS/BSP/Driver/Source/SIC/fmi.h
