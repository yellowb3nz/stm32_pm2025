#include <stdint.h>
#include <stm32f10x.h>

typedef struct {
    GPIO_TypeDef* gpio_base;
    uint32_t gpio_pin;
} GpioPinDef;

static const GpioPinDef Indicator = {GPIOC, 13};
static const GpioPinDef IncrementSwitch = {GPIOA, 0};
static const GpioPinDef DecrementSwitch = {GPIOA, 1};

typedef struct {
    uint16_t prescaler_val;
    uint16_t auto_reload_val;
} TimerSetup;

static TimerSetup timing_config = {7200, 10000};

typedef struct {
    uint8_t increment_prev;
    uint8_t decrement_prev;
} SwitchStatus;

static SwitchStatus switch_states = {1, 1};

static void initialize_gpio_peripherals(void) {
    RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN);

    Indicator.gpio_base->CRH = (Indicator.gpio_base->CRH & ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13))
                               | GPIO_CRH_MODE13_0;
    Indicator.gpio_base->BSRR = (1U << Indicator.gpio_pin);

    uint32_t config_reg = IncrementSwitch.gpio_base->CRL;
    config_reg &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0 | GPIO_CRL_MODE1 | GPIO_CRL_CNF1);
    config_reg |= (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1);
    IncrementSwitch.gpio_base->CRL = config_reg;

    IncrementSwitch.gpio_base->ODR |= (1U << IncrementSwitch.gpio_pin) | (1U << DecrementSwitch.gpio_pin);
}

static void update_timer_prescaler(uint16_t prescaler) {
    TIM2->PSC = prescaler;
    TIM2->EGR = TIM_EGR_UG;
}

static void initialize_timer_module(uint16_t prescaler, uint16_t reload_val) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    update_timer_prescaler(prescaler);
    TIM2->ARR = reload_val;

    TIM2->DIER |= TIM_DIER_UIE;

    NVIC->ISER[0] |= (1 << 28);

    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;
        Indicator.gpio_base->ODR ^= (1U << Indicator.gpio_pin);
    }
}

static uint8_t check_switch_pressed(const GpioPinDef* switch_pin) {
    return (switch_pin->gpio_base->IDR & (1U << switch_pin->gpio_pin)) == 0;
}

static void handle_switch_operations(void) {
    uint8_t increment_current = check_switch_pressed(&IncrementSwitch);
    uint8_t decrement_current = check_switch_pressed(&DecrementSwitch);

    if (increment_current && !switch_states.increment_prev) {
        uint16_t new_prescaler;

        if (timing_config.prescaler_val < 32768) {
            new_prescaler = timing_config.prescaler_val * 2;
        } else {
            new_prescaler = 65535;
        }

        if (new_prescaler != timing_config.prescaler_val) {
            timing_config.prescaler_val = new_prescaler;
            update_timer_prescaler(timing_config.prescaler_val);
        }
    }

    if (decrement_current && !switch_states.decrement_prev) {
        uint16_t new_prescaler;

        if (timing_config.prescaler_val > 2) {
            new_prescaler = timing_config.prescaler_val / 2;
        } else {
            new_prescaler = 1;
        }

        if (new_prescaler != timing_config.prescaler_val) {
            timing_config.prescaler_val = new_prescaler;
            update_timer_prescaler(timing_config.prescaler_val);
        }
    }

    switch_states.increment_prev = increment_current;
    switch_states.decrement_prev = decrement_current;
}

int main(void) {
    initialize_gpio_peripherals();
    initialize_timer_module(timing_config.prescaler_val - 1, timing_config.auto_reload_val - 1);

    while (1) {
        handle_switch_operations();
    }
}