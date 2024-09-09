#include "stm32f030xc.h"

#define LED_PIN_0 7 //PA7
#define LED_PIN_1 0 //PB0
#define LED_PIN_2 1 //PB1

#define JOYSTICK_PIN_UP 11 // PB11
#define JOYSTICK_PIN_DOWN 13 //PB13
#define JOYSTICK_PIN_LEFT 12 //PB12
#define JOYSTICK_PIN_RIGHT 10 // PB10

uint32_t SystemCoreClock = 8000000; 
volatile int toggleRate = 500; // in ms
volatile int currentLED = 0; 

void toggleLED(); // Vorwärtsdeklaration

void configureGPIO() {
    // Enable GPIOA and GPIOB clocks 
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

    //reset the bits 14 and 15 
    GPIOA->MODER &= ~(0x3 << (LED_PIN_0 * 2));
    //set bits 14 and 15 to 01
    GPIOA->MODER |= (0x1 << (LED_PIN_0 * 2));

    GPIOB->MODER &= ~(0x3 << (LED_PIN_1 * 2));
    GPIOB->MODER |= (0x1 << (LED_PIN_1 * 2));

    GPIOB->MODER &= ~(0x3 << (LED_PIN_2 * 2));
    GPIOB->MODER |= (0x1 << (LED_PIN_2 * 2));

    //configure joystick pins as input
    GPIOB->MODER &= ~(0x3 << (JOYSTICK_PIN_UP * 2));
    GPIOB->MODER &= ~(0x3 << (JOYSTICK_PIN_DOWN * 2));
    GPIOB->MODER &= ~(0x3 << (JOYSTICK_PIN_LEFT * 2));
    GPIOB->MODER &= ~(0x3 << (JOYSTICK_PIN_RIGHT * 2));
}

void configureTimer() {
    
    // Enable Timer 6 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    // Configure prescaler to realize a 100 kHz timer counter clock
    TIM6->PSC = SystemCoreClock / 100000 - 1;

    // Configure ARR for initial delay
    TIM6->ARR = (toggleRate * 100) - 1;

    // Configure timer in upcounting mode
    TIM6->CR1 &= ~(TIM_CR1_DIR);
    
    // Enable update interrupt
    TIM6->DIER |= TIM_DIER_UIE;

    // Enable timer
    TIM6->CR1 |= TIM_CR1_CEN;

    // Enable TIM6 interrupt in NVIC
    NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

void TIM6_DAC_IRQHandler() {
    // Check for update interrupt flag
    if (TIM6->SR & TIM_SR_UIF) {
        // Clear update interrupt flag
        TIM6->SR &= ~(TIM_SR_UIF);
        
        // Toggle the current LED
        toggleLED();
    }
}

void toggleLED() {
    switch (currentLED) {
        case 0:
            GPIOA->ODR ^= (1 << LED_PIN_0);
            break;
        case 1:
            GPIOB->ODR ^= (1 << LED_PIN_1);
            break;
        case 2:
            GPIOB->ODR ^= (1 << LED_PIN_2);
            break;
    }
}

void updateToggleRate(int delta) {
    toggleRate += delta;
    if (toggleRate < 50) {
        toggleRate = 50; // Minimum rate 50 ms
    } else if (toggleRate > 1000) {
        toggleRate = 1000; // Maximum rate 1000 ms
    }
    TIM6->ARR = (toggleRate * 100) - 1;
}

void checkJoystick() {
    if (!(GPIOB->IDR & (1 << JOYSTICK_PIN_UP))) {
        updateToggleRate(-50);
    }
    if (!(GPIOB->IDR & (1 << JOYSTICK_PIN_DOWN))) {
        updateToggleRate(50);
    }
    if (!(GPIOB->IDR & (1 << JOYSTICK_PIN_LEFT))) {
        currentLED = (currentLED + 2) % 3;
    }
    if (!(GPIOB->IDR & (1 << JOYSTICK_PIN_RIGHT))) {
        currentLED = (currentLED + 1) % 3;
    }
}

int main(void) {
    configureGPIO();
    configureTimer();

    while (1) {
        checkJoystick();
    }
}
