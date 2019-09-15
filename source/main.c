#include <stdio.h>
#include <string.h>

#include "RTE_Components.h"
#include CMSIS_device_header

#ifdef RTE_DEVICE_HAL_COMMON
    #include "stm32f4xx_hal.h"                  // Device header
#endif

#include "cmsis_os.h"
#include "rl_fs.h"                      // Keil.MDK-Pro::File System:CORE
#include "rl_usb.h"                     // Keil.MDK-Pro::USB:CORE

#include "USBH_MSC.h"

#ifdef RTE_DEVICE_HAL_COMMON
#ifdef RTE_CMSIS_RTOS2_RTX5
/**
  * Override default HAL_GetTick function
  */
uint32_t HAL_GetTick (void)
{
    static uint32_t ticks = 0U;
           uint32_t i;
    
    if (osKernelGetState () == osKernelRunning) 
    {
        return ((uint32_t)osKernelGetTickCount ());
    }
    
    /* If Kernel is not running wait approximately 1 ms then increment 
       and return auxiliary tick counter value */
    for (i = (SystemCoreClock >> 14U); i > 0U; i--)
    {
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    }
    return ++ticks;
}
#endif
#endif

static const char * usb_status_print(usbStatus _state)
{
    switch(_state)
    {
        case usbOK              : return "Function completed with no error";

        case usbTimeout         : return "Function completed; time-out occurred";
        case usbInvalidParameter: return "Invalid Parameter error: a mandatory parameter was missing or specified an incorrect object";

        case usbThreadError     : return "CMSIS-RTOS Thread creation/termination failed";
        case usbTimerError      : return "CMSIS-RTOS Timer creation/deletion failed";
        case usbSemaphoreError  : return "CMSIS-RTOS Semaphore creation failed";
        case usbMutexError      : return "CMSIS-RTOS Mutex creation failed";

        case usbControllerError : return "Controller does not exist";
        case usbDeviceError     : return "Device does not exist";
        case usbDriverError     : return "Driver function produced error";
        case usbDriverBusy      : return "Driver function is busy";
        case usbMemoryError     : return "Memory management function produced error";
        case usbNotConfigured   : return "Device is not configured (is connected)";
        case usbClassErrorADC   : return "Audio Device Class (ADC) error (no device or device produced error)";
        case usbClassErrorCDC   : return "Communication Device Class (CDC) error (no device or device produced error)";
        case usbClassErrorHID   : return "Human Interface Device (HID) error (no device or device produced error)";
        case usbClassErrorMSC   : return "Mass Storage Device (MSC) error (no device or device produced error)";
        case usbClassErrorCustom: return "Custom device Class (Class) error (no device or device produced error)";
        case usbUnsupportedClass: return "Unsupported Class";

        case usbTransferStall   : return "Transfer handshake was stall";
        case usbTransferError   : return "Transfer error";

        case usbUnknownError    : return "Unspecified USB error";
        
        default: return "Unkhown USB error code";
    }
};

/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB:Device
 * Copyright (c) 2004-2014 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HID.c
 * Purpose: USB Device Human Interface Device example program
 *----------------------------------------------------------------------------*/
__NO_RETURN void usbd_handle (void const *argument)
{
#ifdef RTE_DEVICE_HAL_COMMON    
    // Emulate USB disconnect (USB_DP) ->
    static GPIO_InitTypeDef GPIO_InitStruct = 
    {
        .Pin   = GPIO_PIN_12        ,
        .Mode  = GPIO_MODE_OUTPUT_OD,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Pull  = GPIO_NOPULL        ,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_InitStruct.Pin, GPIO_PIN_RESET);
	
	HAL_Delay(10);
    // Emulate USB disconnect (USB_DP) <-
#endif
    
    uint8_t buf[1];

    volatile usbStatus usb_init_status, usb_connect_status;
    
    usb_init_status = USBD_Initialize(0); /* USB Device 0 Initialization */
    printf("USB device init status: %s\r\n", usb_status_print(usb_init_status));
    if (usb_init_status != usbOK) for(;;);
    
    usb_connect_status = USBD_Connect(0); /* USB Device 0 Connect */
    printf("USB device connect status: %s\r\n", usb_status_print(usb_connect_status));
    if (usb_connect_status != usbOK) for(;;);

    for (;;)
    {   /* Loop forever */
        USBD_HID_GetReportTrigger(0, 0, &buf[0], 1);
        osDelay(100); /* 100 ms delay for sampling buttons  */
    }
}

osThreadDef(usbd_handle, osPriorityNormal, 1, 0);

/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB:Host
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    MassStorage.c
 * Purpose: USB Host - Mass Storage example
 *----------------------------------------------------------------------------*/

// Main stack size must be multiple of 8 Bytes
#define USBH_STK_SZ (2048U)
uint64_t usbh_stk[USBH_STK_SZ / 8];
const osThreadAttr_t usbh_handle_attr =
{
    .stack_mem  = &usbh_stk[0],
    .stack_size = sizeof(usbh_stk)
};

/*------------------------------------------------------------------------------
 *        Application
 *----------------------------------------------------------------------------*/

__NO_RETURN void usbh_handle (void *arg) {
    usbStatus usb_status;                 // USB status
    int32_t   msc_status;                 // MSC status
    FILE     *f;                          // Pointer to stream object
    uint8_t   con = 0U;                   // Connection status of MSC(s)
    
    (void)arg;
    
    usb_status = USBH_Initialize (0U);    // Initialize USB Host 0
    printf("USB device init status: %s\r\n", usb_status_print(usb_status));
    if (usb_status != usbOK) for(;;);
    
    for (;;)
    {
        msc_status = USBH_MSC_DriveGetMediaStatus ("U0:");  // Get MSC device status
        
        if (msc_status == USBH_MSC_OK) 
        {
            if (con == 0U)                   // If stick was not connected previously
            {
                con = 1U;                       // Stick got connected
                msc_status = USBH_MSC_DriveMount ("U0:");
                if (msc_status != USBH_MSC_OK) 
                {
                    continue;                     // Handle U0: mount failure
                }
                f = fopen ("Test.txt", "w");    // Open/create file for writing
                if (f == NULL) 
                {
                    continue;                     // Handle file opening/creation failure
                }
                fprintf (f, "USB Host Mass Storage!\n");
                fclose (f);                     // Close file
                
                msc_status = USBH_MSC_DriveUnmount ("U0:");
                
                if (msc_status != USBH_MSC_OK) 
                {
                    continue;                     // Handle U0: dismount failure
                }
            }
        }
        else 
        {
            if (con == 1U)                   // If stick was connected previously
            {
                con = 0U;                       // Stick got disconnected
            }
        }
        osDelay(100U);
    }
}

#ifdef RTE_DEVICE_HAL_COMMON

static void SystemClock_Config(void);
static void Error_Handler(void);

#endif

int main(void)
{
#ifdef RTE_DEVICE_HAL_COMMON
    HAL_Init();

    /* Configure the system clock to 96 MHz */
    SystemClock_Config();
#endif
    // System Initialization
    SystemCoreClockUpdate();

    printf("main runing..\r\n");

    osKernelInitialize();
    // osThreadCreate(osThread(usbd_handle), NULL);
    osThreadNew(usbh_handle, NULL, &usbh_handle_attr);
    osKernelStart();
    
    for (;;);
}

#ifdef RTE_DEVICE_HAL_COMMON

/**
  * @brief System Clock Configuration
  * @note This funtion is generated from CubeMX project
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    
    /** Configure the main internal regulator output voltage 
    */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the CPU, AHB and APB busses clocks 
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 384;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 8;
    RCC_OscInitStruct.PLL.PLLR = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks 
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
    PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLQ;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
    /* User may add here some code to deal with this error */
    
    for(;;){__BKPT(0);}
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
    /* User can add his own implementation to report the file name and line number*/
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
    
    /* Infinite loop */
    for(;;){__BKPT(0);}
}

#endif

#endif /* RTE_DEVICE_HAL_COMMON */
