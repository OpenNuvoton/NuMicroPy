//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////


#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#if MICROPY_LVGL

#include "NuMicro.h"
#include "lvgl/lvgl.h"
#include "driver/include/common.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

typedef struct _TouchADC_obj_t {
    mp_obj_base_t base;
    bool initialized;
    bool calibrated;
} TouchADC_obj_t;


static volatile    uint32_t    g_u32AdcIntFlag_TP;

/*-----------------------------------------------*/
// ADC01 ISR
//
/*-----------------------------------------------*/
void ADC01_IRQHandler(void)
{
    /* Clear the A/D ADINT1 interrupt flag */
    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF1_Msk);

    g_u32AdcIntFlag_TP = 1;

}

/*-----------------------------------------------*/
// Get X Position from Touch Panel (ADC input)
//
/*-----------------------------------------------*/
static uint16_t Get_TP_X(void)
{
    uint16_t    x_adc_in;

    /*=== Get X from ADC input ===*/
    GPIO_SetMode(PB, BIT9, GPIO_MODE_OUTPUT);   // XR
    GPIO_SetMode(PH, BIT5, GPIO_MODE_INPUT);    // YD
    GPIO_SetMode(PH, BIT4, GPIO_MODE_OUTPUT);   // XL
    PB9 = 1;
    PH4 = 0;

    /* Configure the GPB8 ADC analog input pins.  */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk);
    SYS->GPB_MFPH |= SYS_GPB_MFPH_PB8MFP_EADC0_CH8;

    /* Disable the GPB8 digital input path to avoid the leakage current. */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT8);

    /* Configure the sample module 1 for analog input channel 8 and software trigger source.*/
    EADC_ConfigSampleModule(EADC, 1, EADC_SOFTWARE_TRIGGER, 8); // YU

    /* Clear the A/D ADINT1 interrupt flag for safe */
    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF1_Msk);

    /* Enable the sample module 1 interrupt.  */
    EADC_ENABLE_INT(EADC, BIT1);    //Enable sample module A/D ADINT1 interrupt.
    EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 1, BIT1);    //Enable sample module 1 interrupt.
    NVIC_EnableIRQ(ADC1_IRQn);

    /* Reset the ADC interrupt indicator and trigger sample module 1 to start A/D conversion */
    g_u32AdcIntFlag_TP = 0;
    EADC_START_CONV(EADC, BIT1);

    /* Wait ADC interrupt (g_u32AdcIntFlag_TP will be set at IRQ_Handler function) */
    while(g_u32AdcIntFlag_TP == 0);
    x_adc_in = EADC_GET_CONV_DATA(EADC, 1);
    return x_adc_in;

}

/*-----------------------------------------------*/
// Get Y Position from Touch Panel (ADC input)
//
/*-----------------------------------------------*/
static uint16_t Get_TP_Y(void)
{
    uint16_t    y_adc_in;

    /*=== Get Y from ADC input ===*/
    GPIO_SetMode(PB, BIT8, GPIO_MODE_OUTPUT);   // YU
    GPIO_SetMode(PH, BIT5, GPIO_MODE_OUTPUT);   // YD
    GPIO_SetMode(PH, BIT4, GPIO_MODE_INPUT);    // XL
    PB8 = 1;
    PH5 = 0;

    /* Configure the GPB9 ADC analog input pins.  */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk);
    SYS->GPB_MFPH |= SYS_GPB_MFPH_PB9MFP_EADC0_CH9;

    /* Disable the GPB9 digital input path to avoid the leakage current. */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT9);

    /* Configure the sample module 2 for analog input channel 9 and software trigger source.*/
    EADC_ConfigSampleModule(EADC, 2, EADC_SOFTWARE_TRIGGER, 9); // XR

    /* Clear the A/D ADINT1 interrupt flag for safe */
    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF1_Msk);

    /* Enable the sample module 2 interrupt.  */
    EADC_ENABLE_INT(EADC, BIT2);    //Enable sample module A/D ADINT1 interrupt.
    EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 1, BIT2);    //Enable sample module 2 interrupt.
    NVIC_EnableIRQ(ADC1_IRQn);

    /* Reset the ADC interrupt indicator and trigger sample module 2 to start A/D conversion */
    g_u32AdcIntFlag_TP = 0;
    EADC_START_CONV(EADC, BIT2);

    /* Wait ADC interrupt (g_u32AdcIntFlag_TP will be set at IRQ_Handler function) */
    while(g_u32AdcIntFlag_TP == 0);
    y_adc_in = EADC_GET_CONV_DATA(EADC, 2);
    return y_adc_in;
}

static void TouchDriver_Init(void)
{
    /* Enable peripheral clock */
    CLK_EnableModuleClock(EADC_MODULE);

    /* Peripheral clock source */
    CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(8));

    /* Init ADC for TP */
    /* Set input mode as single-end and enable the A/D converter */
    EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);

	Get_TP_X();
	Get_TP_Y();
	
}

static bool TouchDriver_Read(
	lv_indev_drv_t *psIndevDriver,
	lv_indev_data_t *psIndevData
)
{
	static uint16_t s_u16X = 0;
	static uint16_t s_u16Y = 0;
	uint16_t u16X;
	uint16_t u16Y;
	
	u16X = Get_TP_X();
	u16Y = Get_TP_Y();
	
	if((u16X == 0xFFF) || (u16X == 0xFFF)){
		psIndevData->state = LV_INDEV_STATE_REL;
		psIndevData->point.x = s_u16X;
		psIndevData->point.y = s_u16Y;
	}
	else{
		psIndevData->state = LV_INDEV_STATE_PR;
		/*Set the coordinates (if released use the last pressed coordinates)*/
#if 0
		psIndevData->point.x = u16X;
		psIndevData->point.y = u16Y;
		s_u16X = u16X;
		s_u16Y = u16Y;
#else
		psIndevData->point.x = u16Y;
		psIndevData->point.y = u16X;
		s_u16X = u16Y;
		s_u16Y = u16X;
#endif
	}
	
	return false; /*Return `false` because we are not buffering and no more data to read*/
}


STATIC FATFS *lookup_path(const char **path) {
    mp_vfs_mount_t *fs = mp_vfs_lookup_path(*path, path);
    if (fs == MP_VFS_NONE || fs == MP_VFS_ROOT) {
        return NULL;
    }
    // here we assume that the mounted device is FATFS
    return &((fs_user_mount_t*)MP_OBJ_TO_PTR(fs->obj))->fatfs;
}


//////////////////////////////////////////////////////////////////////////////////////////
//		Touch Calibrate 
/////////////////////////////////////////////////////////////////////////////////////////

/*********************
 *      DEFINES
 *********************/
#define CIRCLE_SIZE      20
#define CIRCLE_OFFSET    50
#define TP_MAX_VALUE     5000
#define TOUCH_NUMBER     1 //3

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    TP_CAL_STATE_INIT,
    TP_CAL_STATE_WAIT_TOP_LEFT,
    TP_CAL_STATE_WAIT_TOP_RIGHT,
    TP_CAL_STATE_WAIT_BOTTOM_RIGHT,
    TP_CAL_STATE_WAIT_BOTTOM_LEFT,
    TP_CAL_STATE_WAIT_CENTER,	
    TP_CAL_STATE_WAIT_LEAVE,
    TP_CAL_STATE_READY,
} tp_cal_state_t;

typedef struct
{
    int x[5], xfb[5];
    int y[5], yfb[5];
    int a[7];
} calibration;

/**********************
 *  STATIC VARIABLES
 **********************/
static tp_cal_state_t s_calibrate_state = TP_CAL_STATE_INIT;

static lv_point_t point[5]; /*Calibration points: [0]: top-left; [1]: top-right, [2]: bottom-right, [3]: bottom-left [4]: center*/
static lv_point_t avr[TOUCH_NUMBER]; /*Storage point to calculate average*/

static lv_obj_t * s_prev_scr;
static lv_obj_t * s_calibrate_scr;
static lv_obj_t * s_big_btn;
static lv_obj_t * s_label_main;
static lv_obj_t * s_circ_area;
static lv_obj_t * s_label_coord_tr;
static lv_obj_t * s_label_coord_tl;
static lv_obj_t * s_label_coord_br;
static lv_obj_t * s_label_coord_bl;
static lv_obj_t * s_label_coord_c;

static calibration cal, final_cal;

static int TS_Phy2Log(int *sumx, int *sumy)
{
    int xtemp,ytemp;
	
    xtemp = *sumx;
    ytemp = *sumy;
    *sumx = ( final_cal.a[2] +
              final_cal.a[0]*xtemp +
              final_cal.a[1]*ytemp ) / final_cal.a[6];
    *sumy = ( final_cal.a[5] +
              final_cal.a[3]*xtemp +
              final_cal.a[4]*ytemp ) / final_cal.a[6];
//printf("After X=%d, Y=%d\n",*sumx, *sumy);
    return 1;
}

static int
Read_CalibrateFile(void)
{
	FATFS *psFAT = NULL;
	FRESULT i32Res;
	char *szFilePath = "/flash/TouchCalibrate";
	
	psFAT = lookup_path((const char **)&szFilePath);
	if(psFAT == NULL){
        printf("Unable lookup file path %s\n", szFilePath);
		return -1;
	}
	
	FIL hFile;
	i32Res = f_open(psFAT, &hFile, szFilePath, FA_OPEN_EXISTING | FA_READ);
	if(i32Res != FR_OK){
        printf("Unable open file %s\n", szFilePath);
		return -2;
	}

    size_t i32Cnt;

    i32Res = f_read(&hFile, (char *)&final_cal.a[0], 28, &i32Cnt);
	f_close(&hFile);

    if (i32Res != FR_OK){
        printf("CANNOT read the calibration into file, %d\n", i32Cnt);
        return -3;
    }

	return 0;
}

static int
Write_CalibrateFile(void)
{
	FATFS *psFAT = NULL;
	FRESULT i32Res;
	char *szFilePath = "/flash/TouchCalibrate";
	
	psFAT = lookup_path((const char **)&szFilePath);
	if(psFAT == NULL)
		return -1;
	
	FIL hFile;
	i32Res = f_open(psFAT, &hFile, szFilePath, FA_CREATE_ALWAYS | FA_WRITE);
	if(i32Res != FR_OK){
		return -2;
	}

    size_t i32Cnt;

    i32Res = f_write(&hFile, (char *)&final_cal.a[0], 28, &i32Cnt);
	f_close(&hFile);

    if (i32Res != FR_OK){
        printf("CANNOT write the calibration into file, %d\n", i32Cnt);
        return -3;
    }

	return 0;
}

static int perform_calibration(calibration *cal)
{
    int j;
    float n, x, y, x2, y2, xy, z, zx, zy;
    float det, a, b, c, e, f, i;
    float scaling = 65536.0;
	
// Get sums for matrix
    n = x = y = x2 = y2 = xy = 0;
    for(j=0; j<5; j++)
    {
        n += 1.0f;
        x += (float)cal->x[j];
        y += (float)cal->y[j];
        x2 += (float)(cal->x[j]*cal->x[j]);
        y2 += (float)(cal->y[j]*cal->y[j]);
        xy += (float)(cal->x[j]*cal->y[j]);
    }
// Get determinant of matrix -- check if determinant is too small
    det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
    if(det < 0.1f && det > -0.1f)
    {
        printf("ts_calibrate: determinant is too small -- %f\n",(double)det);
        return 0;
    }

// Get elements of inverse matrix
    a = (x2*y2 - xy*xy)/det;
    b = (xy*y - x*y2)/det;
    c = (x*xy - y*x2)/det;
    e = (n*y2 - y*y)/det;
    f = (x*y - n*xy)/det;
    i = (n*x2 - x*x)/det;

// Get sums for x calibration
    z = zx = zy = 0;
    for(j=0; j<5; j++)
    {
        z += (float)cal->xfb[j];
        zx += (float)(cal->xfb[j]*cal->x[j]);
        zy += (float)(cal->xfb[j]*cal->y[j]);
    }

// Now multiply out to get the calibration for framebuffer x coord
    cal->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
    cal->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
    cal->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));
#if 0 //close
    printf("%f %f %f\n",(a*z + b*zx + c*zy),
              (b*z + e*zx + f*zy),
              (c*z + f*zx + i*zy));
#endif
// Get sums for y calibration
    z = zx = zy = 0;
    for(j=0; j<5; j++)
    {
        z += (float)cal->yfb[j];
        zx += (float)(cal->yfb[j]*cal->x[j]);
        zy += (float)(cal->yfb[j]*cal->y[j]);
    }

// Now multiply out to get the calibration for framebuffer y coord
    cal->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
    cal->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
    cal->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));
#if 0  // closed
    printf("%f %f %f\n",(a*z + b*zx + c*zy),
              (b*z + e*zx + f*zy),
              (c*z + f*zx + i*zy));
#endif
// If we got here, we're OK, so assign scaling to a[6] and return
    cal->a[6] = (int)scaling;
    return 1;
}

static void get_avr_value(lv_point_t * p)
{
	int32_t x_sum = 0;
	int32_t y_sum = 0;
	uint8_t i = 0;
	for(; i < TOUCH_NUMBER ; i++) {
		x_sum += avr[i].x;
		y_sum += avr[i].y;
	}
	p->x = x_sum / TOUCH_NUMBER;
	p->y = y_sum / TOUCH_NUMBER;
}


static void btn_event_cb(lv_obj_t * scr, lv_event_t event)
{
	if(event != LV_EVENT_CLICKED) return;

    lv_disp_t * disp = lv_obj_get_disp(s_prev_scr);
    lv_coord_t hres = lv_disp_get_hor_res(disp);
    lv_coord_t vres = lv_disp_get_ver_res(disp);
	
    static uint8_t touch_nb = TOUCH_NUMBER;	

	if(s_calibrate_state == TP_CAL_STATE_WAIT_TOP_LEFT) {
		char buf[64];
		touch_nb--;
		lv_indev_t * indev = lv_indev_get_act();
		lv_indev_get_point(indev, &avr[touch_nb]);

		if(!touch_nb) {
			touch_nb = TOUCH_NUMBER;
			get_avr_value(&point[0]);
			sprintf(buf, "x: %d\ny: %d", point[0].x, point[0].y);

			cal.x[0] = point[0].x; 
			cal.y[0] = point[0].y;

			s_label_coord_tl = lv_label_create(lv_disp_get_scr_act(disp), NULL);
			lv_label_set_text(s_label_coord_tl, buf);
			sprintf(buf, "Click the circle in\n"
					"upper right-hand corner\n"
					" %u Left", TOUCH_NUMBER);

			cal.xfb[1] = LV_HOR_RES - CIRCLE_OFFSET;
			cal.yfb[1] = CIRCLE_OFFSET;

			lv_obj_set_pos(s_circ_area, cal.xfb[1] - (CIRCLE_SIZE / 2), cal.yfb[1] - (CIRCLE_SIZE / 2));

			s_calibrate_state = TP_CAL_STATE_WAIT_TOP_RIGHT;
		} else {
			sprintf(buf, "Click the circle in\n"
					"upper left-hand corner\n"
					" %u Left", touch_nb);
		}
		lv_label_set_text(s_label_main, buf);
		lv_obj_set_pos(s_label_main, (hres - lv_obj_get_width(s_label_main)) / 2,
					   (vres - lv_obj_get_height(s_label_main)) / 2);
	} else if(s_calibrate_state == TP_CAL_STATE_WAIT_TOP_RIGHT) {
		char buf[64];
		touch_nb--;
		lv_indev_t * indev = lv_indev_get_act();
		lv_indev_get_point(indev, &avr[touch_nb]);

		if(!touch_nb) {
			touch_nb = TOUCH_NUMBER;
			get_avr_value(&point[1]);
			sprintf(buf, "x: %d\ny: %d", point[1].x, point[1].y);

			cal.x[1] = point[1].x; 
			cal.y[1] = point[1].y;

			s_label_coord_tr = lv_label_create(lv_disp_get_scr_act(disp), NULL);
			lv_label_set_text(s_label_coord_tr, buf);
			lv_obj_set_pos(s_label_coord_tr, hres - lv_obj_get_width(s_label_coord_tr), 0);
			sprintf(buf, "Click the circle in\n"
					"lower right-hand corner\n"
					" %u Left", TOUCH_NUMBER);

			cal.xfb[2] = hres - CIRCLE_OFFSET;
			cal.yfb[2] = vres - CIRCLE_OFFSET;

			lv_obj_set_pos(s_circ_area, cal.xfb[2] - (CIRCLE_SIZE / 2) , cal.yfb[2] - (CIRCLE_SIZE / 2));

			s_calibrate_state = TP_CAL_STATE_WAIT_BOTTOM_RIGHT;
		} else {
			sprintf(buf, "Click the circle in\n"
					"upper right-hand corner\n"
					" %u Left", touch_nb);
		}
		lv_label_set_text(s_label_main, buf);
		lv_obj_set_pos(s_label_main, (hres - lv_obj_get_width(s_label_main)) / 2,
					   (vres - lv_obj_get_height(s_label_main)) / 2);

    } else if(s_calibrate_state == TP_CAL_STATE_WAIT_BOTTOM_RIGHT) {
        char buf[64];
        touch_nb--;
        lv_indev_t * indev = lv_indev_get_act();
        lv_indev_get_point(indev, &avr[touch_nb]);

        if(!touch_nb) {
            touch_nb = TOUCH_NUMBER;
            get_avr_value(&point[2]);
            sprintf(buf, "x: %d\ny: %d", point[2].x, point[2].y);

			cal.x[2] = point[2].x; 
			cal.y[2] = point[2].y;

            s_label_coord_br = lv_label_create(lv_disp_get_scr_act(disp), NULL);
            lv_label_set_text(s_label_coord_br, buf);
            lv_obj_set_pos(s_label_coord_br, hres - lv_obj_get_width(s_label_coord_br),
                           vres - lv_obj_get_height(s_label_coord_br));
            sprintf(buf, "Click the circle in\n"
                    "lower left-hand corner\n"
                    " %u Left", TOUCH_NUMBER);

			cal.xfb[3] = CIRCLE_OFFSET;
			cal.yfb[3] = LV_VER_RES - CIRCLE_OFFSET;

            lv_obj_set_pos(s_circ_area, cal.xfb[3] - (CIRCLE_SIZE / 2), cal.yfb[3] - (CIRCLE_SIZE / 2));

            s_calibrate_state = TP_CAL_STATE_WAIT_BOTTOM_LEFT;
        } else {
            sprintf(buf, "Click the circle in\n"
                    "lower right-hand corner\n"
                    " %u Left", touch_nb);
        }
        lv_label_set_text(s_label_main, buf);
        lv_obj_set_pos(s_label_main, (hres - lv_obj_get_width(s_label_main)) / 2,
                       (vres - lv_obj_get_height(s_label_main)) / 2);
    }else if(s_calibrate_state == TP_CAL_STATE_WAIT_BOTTOM_LEFT) {
        char buf[64];
        touch_nb--;
        lv_indev_t * indev = lv_indev_get_act();
        lv_indev_get_point(indev, &avr[touch_nb]);

        if(!touch_nb) {
            touch_nb = TOUCH_NUMBER;
            get_avr_value(&point[3]);
            sprintf(buf, "x: %d\ny: %d", point[3].x, point[3].y);

			cal.x[3] = point[3].x; 
			cal.y[3] = point[3].y;

            s_label_coord_bl = lv_label_create(lv_disp_get_scr_act(disp), NULL);
            lv_label_set_text(s_label_coord_bl, buf);
            lv_obj_set_pos(s_label_coord_bl, 0, vres - lv_obj_get_height(s_label_coord_bl));
            sprintf(buf, "Click the circle in\n"
                    "center\n"
                    " %u Left", TOUCH_NUMBER);

			cal.xfb[4] = LV_HOR_RES / 2;
			cal.yfb[4] = LV_VER_RES / 2;

            lv_obj_set_pos(s_circ_area, cal.xfb[4], cal.yfb[4]);

            lv_obj_set_pos(s_circ_area, (LV_HOR_RES / 2) - (CIRCLE_SIZE / 2), (LV_VER_RES / 2) - (CIRCLE_SIZE / 2));
            s_calibrate_state = TP_CAL_STATE_WAIT_CENTER;
        } else {
            sprintf(buf, "Click the circle in\n"
                    "lower left-hand corner\n"
                    " %u Left", touch_nb);
        }
        lv_label_set_text(s_label_main, buf);
        lv_obj_set_pos(s_label_main, (hres - lv_obj_get_width(s_label_main)) / 2,
                       (vres - lv_obj_get_height(s_label_main)) / 2);

    } else if(s_calibrate_state == TP_CAL_STATE_WAIT_CENTER) {
        char buf[64];
        touch_nb--;
        lv_indev_t * indev = lv_indev_get_act();
        lv_indev_get_point(indev, &avr[touch_nb]);

        if(!touch_nb) {
            touch_nb = TOUCH_NUMBER;
            get_avr_value(&point[4]);
            sprintf(buf, "x: %d\ny: %d", point[4].x, point[4].y);

			cal.x[4] = point[4].x; 
			cal.y[4] = point[4].y;

            s_label_coord_c = lv_label_create(lv_disp_get_scr_act(disp), NULL);
            lv_label_set_text(s_label_coord_c, buf);
            lv_obj_set_pos(s_label_coord_c, hres / 2, vres /2 + 50);
            sprintf(buf, "Click the screen\n"
                    "to leave calibration");
            lv_obj_del(s_circ_area);
            s_calibrate_state = TP_CAL_STATE_WAIT_LEAVE;
        } else {
            sprintf(buf, "Click the circle in\n"
                    "center\n"
                    " %u Left", touch_nb);
        }
        lv_label_set_text(s_label_main, buf);
        lv_obj_set_pos(s_label_main, (hres - lv_obj_get_width(s_label_main)) / 2,
                       (vres - lv_obj_get_height(s_label_main)) / 2);

    } else if(s_calibrate_state == TP_CAL_STATE_WAIT_LEAVE) {
		lv_disp_load_scr(s_prev_scr);

		/*
		 * TODO Process 'p' points here to calibrate the touch pad
		 * Offset will be: CIRCLE_SIZE/2 + CIRCLE_OFFSET
		 */
		int i;
		if (perform_calibration (&cal))
		{
			printf ("Calibration constants: ");
			for (i = 0; i < 7; i++) printf("%d ", cal.a [i]);
			printf("\n");
		}
		else
		{
			printf("Calibration failed.\n");
			i = -1;
		}

		final_cal.a[0] = cal.a[1];
		final_cal.a[1] = cal.a[2];
		final_cal.a[2] = cal.a[0];
		final_cal.a[3] = cal.a[4];
		final_cal.a[4] = cal.a[5];
		final_cal.a[5] = cal.a[3];
		final_cal.a[6] = cal.a[6];

		s_calibrate_state = TP_CAL_STATE_READY;

    } else if(s_calibrate_state == TP_CAL_STATE_READY) {

    }
}

static int32_t 
Touch_Calibrate_Create(void)
{
    s_calibrate_state = TP_CAL_STATE_INIT;

    s_prev_scr = lv_disp_get_scr_act(NULL);

    s_calibrate_scr = lv_obj_create(NULL, NULL);
	
	if(s_calibrate_scr == NULL){
		printf("Unable create touch calibrate screen \n");
		return -1;
	}

    lv_obj_set_size(s_calibrate_scr, TP_MAX_VALUE, TP_MAX_VALUE);
    lv_disp_load_scr(s_calibrate_scr);

    /*Create a big transparent button screen to receive clicks*/
    s_big_btn = lv_btn_create(lv_disp_get_scr_act(NULL), NULL);
    lv_obj_set_size(s_big_btn, TP_MAX_VALUE, TP_MAX_VALUE);
    lv_btn_set_style(s_big_btn, LV_BTN_STYLE_REL, &lv_style_transp);
    lv_btn_set_style(s_big_btn, LV_BTN_STYLE_PR, &lv_style_transp);
    lv_obj_set_event_cb(s_big_btn, btn_event_cb);
    lv_btn_set_layout(s_big_btn, LV_LAYOUT_OFF);

    s_label_main = lv_label_create(lv_disp_get_scr_act(NULL), NULL);
    char buf[64];
    sprintf(buf, "Click the circle in\n"
            "upper left-hand corner\n"
            "%u left", TOUCH_NUMBER);
    lv_label_set_text(s_label_main, buf);
    lv_label_set_align(s_label_main, LV_LABEL_ALIGN_CENTER);

    lv_coord_t hres = lv_disp_get_hor_res(NULL);
    lv_coord_t vres = lv_disp_get_ver_res(NULL);

    lv_obj_set_pos(s_label_main, (hres - lv_obj_get_width(s_label_main)) / 2,
                   (vres - lv_obj_get_height(s_label_main)) / 2);


    static lv_style_t style_circ;
    lv_style_copy(&style_circ, &lv_style_pretty_color);
    style_circ.body.radius = LV_RADIUS_CIRCLE;

    s_circ_area = lv_obj_create(lv_disp_get_scr_act(NULL), NULL);
    lv_obj_set_size(s_circ_area, CIRCLE_SIZE, CIRCLE_SIZE);
    lv_obj_set_style(s_circ_area, &style_circ);
    lv_obj_set_click(s_circ_area, false);

	cal.xfb[0] = CIRCLE_OFFSET;
	cal.yfb[0] = CIRCLE_OFFSET;
    lv_obj_set_pos(s_circ_area, cal.xfb[0] - (CIRCLE_SIZE / 2), cal.yfb[0] - (CIRCLE_SIZE / 2));
    s_calibrate_state = TP_CAL_STATE_WAIT_TOP_LEFT;

	return 0;
}


/******************************************************************************/
/* MicroPython bindings                                                       */


STATIC const mp_obj_type_t TouchADC_type;

STATIC TouchADC_obj_t TouchADC_obj = {
	.base = {(mp_obj_type_t *)&TouchADC_type},
	.initialized = false,
	.calibrated = false,
};

STATIC mp_obj_t TouchADC_init(
	mp_obj_t self_in
)
{
	TouchADC_obj_t *self = (TouchADC_obj_t *)self_in;
	if(!self->initialized){
		TouchDriver_Init();
		
		//Set default calibrate value
		final_cal.a[0] = 6677;
		final_cal.a[1] = -986;
		final_cal.a[2] = -3173776;
		final_cal.a[3] = -151;
		final_cal.a[4] = 4643;
		final_cal.a[5] = -2143028;
		final_cal.a[6] = 65536;

		self->initialized = true;
	}

	if(Read_CalibrateFile() == 0){
		//Already calibrate
		self->calibrated = true;
	}
	
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(TouchADC_init_obj, TouchADC_init);

STATIC bool TouchADC_Read(
	lv_indev_drv_t *psIndevDriver,
	lv_indev_data_t *psIndevData
)
{
	int32_t i32X, i32Y;  

	TouchDriver_Read(psIndevDriver, psIndevData);

	if((s_calibrate_state != TP_CAL_STATE_READY) &&
		(s_calibrate_state != TP_CAL_STATE_INIT))
	{
		return false;
	}

	i32X = psIndevData->point.x;
	i32Y = psIndevData->point.y;
	
	TS_Phy2Log((int *)&i32X, (int *)&i32Y);

	psIndevData->point.x = i32X;
	psIndevData->point.y = i32Y;

	return false;
}

STATIC mp_obj_t TouchADC_iscalibrated(mp_obj_t self_in) {
	TouchADC_obj_t *self = (TouchADC_obj_t *)self_in;

    return mp_obj_new_bool(self->calibrated);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(TouchADC_iscalibrated_obj, TouchADC_iscalibrated);

STATIC mp_obj_t TouchADC_calibrate(mp_obj_t self_in) {
	TouchADC_obj_t *self = (TouchADC_obj_t *)self_in;
	
	if(Touch_Calibrate_Create() == 0){
		self->calibrated = false;
		while(1){
			lv_task_handler();
			if(s_calibrate_state == TP_CAL_STATE_READY){
				self->calibrated = true;
				mp_hal_delay_ms(LV_DISP_DEF_REFR_PERIOD);
				lv_task_handler();
				Write_CalibrateFile();
				lv_obj_del(s_big_btn);
				lv_obj_del(s_label_main);

				lv_obj_del(s_label_coord_tr);
				lv_obj_del(s_label_coord_tl);
				lv_obj_del(s_label_coord_br);
				lv_obj_del(s_label_coord_bl);
				lv_obj_del(s_label_coord_c);
				lv_obj_del(s_calibrate_scr);
				break;
			}
		}
	}

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(TouchADC_calibrate_obj, TouchADC_calibrate);


DEFINE_PTR_OBJ(TouchADC_Read);


STATIC const mp_rom_map_elem_t TouchADC_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&TouchADC_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_iscalibrated), MP_ROM_PTR(&TouchADC_iscalibrated_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate), MP_ROM_PTR(&TouchADC_calibrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&PTR_OBJ(TouchADC_Read)) },
};

STATIC MP_DEFINE_CONST_DICT(TouchADC_locals_dict, TouchADC_locals_dict_table);

STATIC mp_obj_t TouchADC_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	TouchADC_obj_t *self = &TouchADC_obj;

    if (self->initialized) {
        return (mp_obj_t)self;
    }

	//TODO something?
#if 0
    enum{
         ARG_width,
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_width,MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int=320}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

#endif

	return (mp_obj_t)self;
}

STATIC const mp_obj_type_t TouchADC_type = {
    { &mp_type_type },
    .name = MP_QSTR_TouchADC,
    //.print = TouchADC_print,
    .make_new = TouchADC_make_new,
    .locals_dict = (mp_obj_dict_t*)&TouchADC_locals_dict,
};

STATIC const mp_rom_map_elem_t TouchADC_globals_table[] = {

	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_TouchADC) },
	{ MP_ROM_QSTR(MP_QSTR_touch), (mp_obj_t)&TouchADC_type},
};

STATIC MP_DEFINE_CONST_DICT(TouchADC_module_globals, TouchADC_globals_table);

const mp_obj_module_t mp_module_TouchADC = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&TouchADC_module_globals,
};

#endif


