#include <stdint.h>
#include "stm32f10x.h"

// Определение минимальной и максимальной частоты мигания светодиода (в Гц)
#define MIN_FREQ 1
#define MAX_FREQ 64

// Глобальные переменные:
volatile uint32_t current_freq = 1;      // Текущая частота мигания (начинаем с 1 Гц)
volatile uint8_t button_a_pressed = 0;   // Флаг нажатия кнопки A (увеличение частоты)
volatile uint8_t button_b_pressed = 0;   // Флаг нажатия кнопки B (уменьшение частоты)

int main(void) {
	// Включение тактирования портов B и C
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;

	// Настройка вывода PC13 (светодиод) как выход push-pull
	GPIOC->CRH &= ~GPIO_CRH_CNF13;       // Очищаем биты конфигурации
	GPIOC->CRH |= GPIO_CRH_MODE13;       // Устанавливаем режим output (50 МГц)

	// Настройка выводов PB0 и PB1 (кнопки) как входы с pull-up
	GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_CNF1 | GPIO_CRL_MODE0 | GPIO_CRL_MODE1); // Очищаем биты
	GPIOB->CRL |= GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1;  // Режим input with pull-up/pull-down
	GPIOB->ODR |= GPIO_ODR_ODR0 | GPIO_ODR_ODR1;      // Активируем pull-up резисторы

	// Бесконечный цикл программы
	while (1) {
		// Обработка кнопки A (PB0) - увеличение частоты
		if (((GPIOB->IDR & GPIO_IDR_IDR0) == 0) && !button_a_pressed) {
			button_a_pressed = 1;  // Устанавливаем флаг нажатия
			if (current_freq < MAX_FREQ) current_freq *= 2;  // Удваиваем частоту (до макс. значения)
		}

		// Обработка кнопки B (PB1) - уменьшение частоты
		if (((GPIOB->IDR & GPIO_IDR_IDR1) == 0) && !button_b_pressed) {
			button_b_pressed = 1;  // Устанавливаем флаг нажатия
			if (current_freq > MIN_FREQ) current_freq /= 2;  // Уменьшаем частоту вдвое (до мин. значения)
		}

		// Сброс флагов нажатия при отпускании кнопок
		if (!((GPIOB->IDR & GPIO_IDR_IDR0) == 0)) button_a_pressed = 0;
		if (!((GPIOB->IDR & GPIO_IDR_IDR1) == 0)) button_b_pressed = 0;

		// Переключение состояния светодиода на PC13 (XOR для инверсии бита)
		GPIOC->ODR ^= GPIO_ODR_ODR13;

		// Задержка, зависящая от текущей частоты
		// Чем выше частота - тем меньше задержка
		for (uint32_t i = 0; i < 1000000 / current_freq; i++) __NOP();  // NOP - операция без действия
	}
}
