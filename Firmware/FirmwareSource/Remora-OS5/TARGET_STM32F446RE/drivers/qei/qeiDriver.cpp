#include "mbed.h"

#include "qeiDriver.h"

QEIdriver::QEIdriver() :
    qeiIndex(NC)
{
    this->hasIndex = false;
    this->init();
}


QEIdriver::QEIdriver(bool hasIndex) :
    hasIndex(hasIndex),
//    qeiIndex(PE_13)
    qeiIndex(PA_2)
{
    this->hasIndex = true;
    //this->irq = EXTI15_10_IRQn;
    this->irq = EXTI2_IRQn;

    this->init();

    qeiIndex.rise(callback(this, &QEIdriver::interruptHandler));
    //NVIC_EnableIRQ(this->irq);
    HAL_NVIC_SetPriority(this->irq, 0, 0);
}


void QEIdriver::interruptHandler()
{
    this->indexDetected = true;
    this->indexCount = this->get();
}


uint32_t QEIdriver::get()
{
    return __HAL_TIM_GET_COUNTER(&htim);
}


// reference https://os.mbed.com/users/gregeric/code/Nucleo_Hello_Encoder/

void QEIdriver::init()
{
    printf("Initialising hardware QEI module on TIM2\n");

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM2 GPIO Configuration
        PA_0     ------> TIM2_CH1
        PA_1     ------> TIM2_CH2
        */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    this->htim.Instance = TIM2;
    this->htim.Init.Prescaler = 0;
    this->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    this->htim.Init.Period = 0xffffffff; // 32-bit count
    this->htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    this->htim.Init.RepetitionCounter = 0;

    this->sConfig.EncoderMode = TIM_ENCODERMODE_TI12; //x4; both edges on both inputs
//    this->sConfig.EncoderMode = TIM_ENCODERMODE_TI1; //x2; both edges on input 1
//    this->sConfig.EncoderMode = TIM_ENCODERMODE_TI2; //x2; both edges on input 2

    this->sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    this->sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    this->sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    this->sConfig.IC1Filter = 0;

    this->sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    this->sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    this->sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    this->sConfig.IC2Filter = 0;

    if (HAL_TIM_Encoder_Init(&this->htim, &this->sConfig) != HAL_OK)
    {
        printf("Couldn't Init Encoder\r\n");
    }

    this->sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    this->sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&this->htim, &this->sMasterConfig);

    if (HAL_TIM_Encoder_Start(&this->htim, TIM_CHANNEL_ALL)!=HAL_OK)
    {
        printf("Couldn't Start Encoder\r\n");
    }

    //HAL_TIM_Encoder_MspInit(&this->htim);
}

void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef* htim_encoder)
{
    printf("Encoder MspInit\n");
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(htim_encoder->Instance==TIM2)
    {
        printf("Enabling TIM2 for QEI");
        __HAL_RCC_TIM2_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM2 GPIO Configuration
        PA_0     ------> TIM2_CH1
        PA_1     ------> TIM2_CH2
        */
        GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}


