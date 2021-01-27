/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "N9H26_VideoIn.h"
extern VINDEV_T nvt_vin0;
extern VINDEV_T nvt_vin1;
INT32 register_vin_device(UINT32 u32port, VINDEV_T* pVinDev)
{
	if(u32port==1)
		*pVinDev = nvt_vin0;
	else if(u32port==2)
		*pVinDev = nvt_vin1;
	else 	
		return -1;
	return Successful;	
}
