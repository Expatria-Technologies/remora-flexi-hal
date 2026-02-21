#ifndef IRQ_HANDLERS_H
#define IRQ_HANDLERS_H

#include "remora-core/interrupt/interrupt.h"
#include "stm32f4xx_hal.h"
#include "remora-hal/STM32F4_EthComms.h"

//#define THREAD_DEBUG
#ifdef THREAD_DEBUG
extern Pin* thread_debug;
#define BASE_THREAD_DEBUG
//#define SERVO_THREAD_DEBUG
#endif

extern "C" {

    void EXTI4_IRQHandler() {
        if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET) {
            __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);
            Interrupt::InvokeHandler(EXTI4_IRQn);
        }
    }

    void EXTI9_5_IRQHandler(void) // For QEI, hardwired to use PA_8 (EXTI8) for index. 
    {
        if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8) != RESET) {
            __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
            Interrupt::InvokeHandler(EXTI9_5_IRQn);
        }        
    }    

    // macro for automatically creating the DMA Stream IRQ handlers, e.g DMA2_Stream0_IRQHandler()
    #define DMA_STREAM_IRQ_HANDLER(DMA_num, DMA_stream, IRQ_n)      \
    void DMA##DMA_num##_Stream##DMA_stream##_IRQHandler(void) {     \
        Interrupt::InvokeHandler(IRQ_n);                            \
    }

    DMA_STREAM_IRQ_HANDLER(2, 0, DMA2_Stream0_IRQn) // SPI 1 RX
    DMA_STREAM_IRQ_HANDLER(1, 3, DMA1_Stream3_IRQn) // SPI 2 RX
    DMA_STREAM_IRQ_HANDLER(1, 0, DMA1_Stream0_IRQn) // SPI 3 RX
    
    DMA_STREAM_IRQ_HANDLER(2, 3, DMA2_Stream3_IRQn) // SPI 1 TX 
    DMA_STREAM_IRQ_HANDLER(1, 4, DMA1_Stream4_IRQn) // SPI 2 TX
    DMA_STREAM_IRQ_HANDLER(1, 5, DMA1_Stream5_IRQn) // SPI 3 TX

    // macro for automatically creating the SPI IRQ handlers. Should be unused by SPI comms, only for ETH comms
    #define SPI_IRQ_HANDLER(SPI_IRQ_FUNCTION_NAME, SPI_IRQ_NUM) \
    void SPI_IRQ_FUNCTION_NAME(void) {                          \
        Interrupt::InvokeHandler(SPI_IRQ_NUM);                  \
    }

    SPI_IRQ_HANDLER(SPI1_IRQHandler, SPI1_IRQn)        
    SPI_IRQ_HANDLER(SPI2_IRQHandler, SPI2_IRQn)
    SPI_IRQ_HANDLER(SPI3_IRQHandler, SPI3_IRQn)  

    void TIM2_IRQHandler() {
        if (TIM2->SR & TIM_SR_UIF) {
            TIM2->SR &= ~TIM_SR_UIF;
            Interrupt::InvokeHandler(TIM2_IRQn);       
            
            #ifdef SERVO_THREAD_DEBUG
                thread_debug->set(true);  // a bit crude, but your oscilloscope should be able to used this to confirm the real time thread signal.
                thread_debug->set(false);                
            #endif
        }
    }
    void TIM3_IRQHandler() {
        if (TIM3->SR & TIM_SR_UIF) {
            TIM3->SR &= ~TIM_SR_UIF;
            Interrupt::InvokeHandler(TIM3_IRQn);           
            
            #ifdef BASE_THREAD_DEBUG
                thread_debug->set(true); 
                thread_debug->set(false);                    
            #endif            
        }
    }
    
    // void TIM4_IRQHandler() { // SerialThread is disabled in this build. 
    //     if (TIM4->SR & TIM_SR_UIF) {
    //         TIM4->SR &= ~TIM_SR_UIF;
    //         Interrupt::InvokeHandler(TIM4_IRQn);
    //     }
    // }

} // extern "C"

#endif // IRQ_HANDLERS_H