/* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of Nuvoton Technology Corp. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#ifndef __NUMEDIA_SAL_OS_H__
#define __NUMEDIA_SAL_OS_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \anchor          NUMEDIA_SAL_PORTS
 * \name            NuMedia system ports
 * \{
 *
 * List of already available system ports.
 * Configure \ref NUMEDIA_CFG_SYS_PORT with one of these values to use preconfigured ports
 */

#define NUMEDIA_SAL_OS_PORT_NONE                       0   /*!< Without SAL */
#define NUMEDIA_SAL_OS_PORT_FREERTOS_POSIX             1   /*!< FreeRTOS based port */
#define NUMEDIA_SAL_OS_PORT_USER                       99  /*!< User custom implementation.
                                                    When port is selected to user mode, user must provide "NuMedia_SAL_User.h" file,
                                                    which is not provided with library.
                                                    */

#define NUMEDIA_CFG_SAL_OS_PORT    NUMEDIA_SAL_OS_PORT_FREERTOS_POSIX

// Boolean type
#if !defined(__cplusplus) && !defined(NM_NBOOL)
	#if defined(bool) && (false != 0 || true != !false)
		#warning bool is redefined: false(0), true(!0)
	#endif

	#undef	bool
	#undef	false
	#undef	true
	#define bool	uint8_t
	#define false	0
	#define true	(!false)

#endif // #if !defined(__cplusplus) && !defined(NM_NBOOL)


#if NUMEDIA_CFG_SAL_OS_PORT == NUMEDIA_SAL_OS_PORT_NONE
/* For a totally minimal and standalone system, we provide null
   definitions of the sys_ functions. */



#elif NUMEDIA_CFG_SAL_OS_PORT == NUMEDIA_SAL_OS_PORT_FREERTOS_POSIX

#include "FreeRTOS.h"
#include "task.h"

#define posixconfigENABLE_CLOCK_T                1 /**< clock_t in sys/types.h */
#define posixconfigENABLE_CLOCKID_T              1 /**< clockid_t in sys/types.h */
#define posixconfigENABLE_MODE_T                 1 /**< mode_t in sys/types.h */
#define posixconfigENABLE_PID_T                  1 /**< pid_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_ATTR_T         1 /**< pthread_attr_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_COND_T         1 /**< pthread_cond_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_CONDATTR_T     1 /**< pthread_condattr_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_MUTEX_T        1 /**< pthread_mutex_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_MUTEXATTR_T    1 /**< pthread_mutexattr_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_T              1 /**< pthread_t in sys/types.h */
#define posixconfigENABLE_SSIZE_T                1 /**< ssize_t in sys/types.h */
#define posixconfigENABLE_TIME_T                 1 /**< time_t in sys/types.h */
#define posixconfigENABLE_TIMER_T                1 /**< timer_t in sys/types.h */
#define posixconfigENABLE_USECONDS_T             1 /**< useconds_t in sys/types.h */
#define posixconfigENABLE_TIMESPEC               1 /**< struct timespec in time.h */
#define posixconfigENABLE_ITIMERSPEC             1 /**< struct itimerspec in time.h */
#define posixconfigENABLE_SEM_T                  1 /**< struct sem_t in semaphore.h */
#define posixconfigENABLE_PTHREAD_BARRIER_T      1 /**< pthread_barrier_t in sys/types.h */


#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/semaphore.h"
#include "FreeRTOS_POSIX/time.h"
#include "FreeRTOS_POSIX/unistd.h"

#endif

#ifdef __cplusplus
}
#endif

#endif
