/**************************************************************************//**
 * @file     nu_bitutil.h
 * @version  V0.01
 * @brief    Bit utility
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef NU_BIT_UTIL_H
#define NU_BIT_UTIL_H

//#if defined(__ICCARM__) && ( defined(__CORTEX_M0) || defined(__CORTEX_M0P) || defined(__CORTEX_M23))
//#include <arm_math.h>
//#endif
//#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

__STATIC_INLINE int nu_clz(uint32_t x)
{
    return __CLZ(x);
}

__STATIC_INLINE int nu_clo(uint32_t x)
{
    return nu_clz(~x);
}

__STATIC_INLINE int nu_ctz(uint32_t x)
{
    int c = __CLZ(x & -x);
    return x ? 31 - c : c;
}

__STATIC_INLINE int nu_cto(uint32_t x)
{
    return nu_ctz(~x);
}


__STATIC_INLINE uint16_t nu_get16_le(const uint8_t *pos)
{
	uint16_t val;
	
	val = *pos ++;
	val += (*pos << 8);
	
	return val;
}

__STATIC_INLINE void nu_set16_le(uint8_t *pos, uint16_t val)
{
	*pos ++ = val & 0xFF;
	*pos = val >> 8;
}

__STATIC_INLINE uint32_t nu_get32_le(const uint8_t *pos)
{
	uint32_t val;
	
	val = *pos ++;
	val += (*pos ++ << 8);
	val += (*pos ++ << 16);
	val += (*pos ++ << 24);
	
	return val;
}

__STATIC_INLINE void nu_set32_le(uint8_t *pos, uint32_t val)
{
	*pos ++ = val & 0xFF;
	*pos ++ = (val >> 8) & 0xFF;
	*pos ++ = (val >> 16) & 0xFF;
	*pos = (val >> 24) & 0xFF;
}

__STATIC_INLINE uint16_t nu_get16_be(const uint8_t *pos)
{
	uint16_t val;
	
	val = *pos ++;
	val <<= 8;
	val += *pos;
	
	return val;
}

__STATIC_INLINE void nu_set16_be(uint8_t *pos, uint16_t val)
{
	*pos ++ = val >> 8;
	*pos = (val & 0xFF);
}

__STATIC_INLINE uint32_t nu_get32_be(const uint8_t *pos)
{
	uint32_t val;
	
	val = *pos ++;
	val <<= 8;
	val += *pos ++;
	val <<= 8;
	val += *pos ++;
	val <<= 8;
	val += *pos;
	
	return val;
}

__STATIC_INLINE void nu_set32_be(uint8_t *pos, uint32_t val)
{
	*pos ++ = val >> 24;
	*pos ++ = val >> 16;
	*pos ++ = val >> 8;
	*pos ++ = (val & 0xFF);
}

#ifdef __cplusplus
}
#endif

#endif
