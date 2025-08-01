
typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

NORETURN void mp_hal_raise(HAL_StatusTypeDef status);

#if MICROPY_KBD_EXCEPTION
#include "shared/runtime/interrupt_char.h"
#else
static inline void mp_hal_set_interrupt_char(char c) {}
#endif

#define mp_hal_delay_us_fast(us) mp_hal_delay_us(us)

#include "mods/classPin.h"

#define MP_HAL_PIN_FMT                  "%q"
#define mp_hal_pin_name(p) (p)

#define MP_HAL_PIN_MODE_INPUT			GPIO_MODE_INPUT
#define MP_HAL_PIN_MODE_OUTPUT			GPIO_MODE_OUTPUT
#define MP_HAL_PIN_MODE_OPEN_DRAIN		GPIO_MODE_OPEN_DRAIN
#define MP_HAL_PIN_MODE_ALT_OPEN_DRAIN  GPIO_MODE_AF_OD
#define MP_HAL_PIN_MODE_ALT_PUSH_PULL	GPIO_MODE_AF_PP

#define mp_hal_pin_obj_t const pin_obj_t*
#define mp_hal_pin_open_drain(p) mp_hal_pin_config((p), MP_HAL_PIN_MODE_OPEN_DRAIN, 0)
#define mp_hal_get_pin_obj(o)   pin_find(o)
#define mp_hal_pin_od_low(p)    mp_hal_pin_low(p)
#define mp_hal_pin_od_high(p)   mp_hal_pin_high(p)

void mp_hal_pin_config(mp_hal_pin_obj_t pin, uint32_t mode, const pin_af_obj_t *af_obj);
bool mp_hal_pin_config_alt(mp_hal_pin_obj_t pin, uint32_t mode, uint8_t fn, uint8_t unit);

#define mp_hal_pin_output(p)    mp_hal_pin_config((p), MP_HAL_PIN_MODE_OUTPUT, 0)
#define mp_hal_pin_input(p)     mp_hal_pin_config((p), MP_HAL_PIN_MODE_INPUT, 0)

void mp_hal_ticks_cpu_enable(void);
static inline mp_uint_t mp_hal_ticks_cpu(void)
{
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        mp_hal_ticks_cpu_enable();
    }
    return DWT->CYCCNT;
}

// Atomic section helpers.
#define MICROPY_BEGIN_ATOMIC_SECTION()     disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)  enable_irq(state)

