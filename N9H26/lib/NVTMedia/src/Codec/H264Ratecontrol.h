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

#ifndef _RATECONTROL_H
#define _RATECONTROL_H

#ifdef int64_t
#undef int64_t
#endif

typedef long long int64_t;
#define rc_MAX_QUANT  52
#define rc_MIN_QUANT  0

typedef struct
{
	int rtn_quant;
	//long long frames;
	int64_t frames;
	//long long total_size;
	double total_size;
	double framerate;
	int target_rate;
	short max_quant;
	short min_quant;
	//long long last_change;
	//long long quant_sum;
	int64_t last_change;
	int64_t quant_sum;
//	double quant_error[32];
	double quant_error[rc_MAX_QUANT];
	double avg_framesize;
	double target_framesize;
	double sequence_quality;
	int averaging_period;
	int reaction_delay_factor;
	int buffer;
	unsigned int IPInterval;
    unsigned int IPIntervalCnt;
	int pre_rtn_quant;
}H264RateControl;

//#define RC_DELAY_FACTOR         16
#define RC_DELAY_FACTOR         4
#define RC_AVERAGING_PERIOD     100
#define RC_BUFFER_SIZE_QUALITY  100 //quality sensitive, recommended
#define RC_BUFFER_SIZE_BITRATE  10  //bit rate sensitive, not recommended

#define quality_const (double)((double)2/(double)rc_MAX_QUANT)

void H264RateControlInit(H264RateControl *rate_control,
				unsigned int target_rate,
				unsigned int reaction_delay_factor,
				unsigned int averaging_period,
				unsigned int buffer,
				float framerate,
				int max_quant,
				int min_quant,
				unsigned int initq,
				unsigned int IPInterval);
				
void H264RateControlUpdate(H264RateControl *rate_control,
				  short quant,
				  int frame_size,
				  int keyframe);



#endif
