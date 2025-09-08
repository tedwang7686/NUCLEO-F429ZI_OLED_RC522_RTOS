
/**
 * @file    oled_rtos_task.c
 * @author  Ted Wang
 * @date    2025-08-01
 * @brief   RTOS task for OLED display control (CMSIS-RTOS v2)
 *
 * @details
 * Implements a FreeRTOS/CMSIS-RTOS v2 task for OLED display update using u8g2 library.
 * The task receives rc522 sensor data from a message queue and updates the display accordingly.
 */

/* Includes ------------------------------------------------------------------*/
#include "oled_rtos_task.h"
#include "rc522_rtos_task.h"
#include "main.h"
#include "oled_driver.h"
#include <string.h>
#include <stdio.h>


/**
 * @brief OLED RTOS task handle.
 */
static osThreadId_t oled_task_handle;

/**
 * @brief Queue handle for RC522 info updates.
 */
osMessageQueueId_t display_rc522_info_queue;

/**
 * @brief UART3 handle for debug/error output (defined elsewhere).
 */
extern UART_HandleTypeDef huart3;

/**
 * @brief OLED RTOS display task function (thread entry point).
 * @param argument Unused (required by CMSIS-RTOS API)
 */
static void OLED_Display_Task(void *argument);



/**
 * @brief  Initialize the OLED display RTOS task and message queue.
 *
 * This function creates the message queue for rc522 info updates and starts the OLED display task.
 * Call once during system initialization before the RTOS kernel starts.
 *
 * @note If queue or task creation fails, outputs error via UART3 and calls Error_Handler().
 */
void OLED_Task_Init(void)
{
    display_rc522_info_queue = osMessageQueueNew(RC522_QUEUE_SIZE, sizeof(RC522_Data_t), NULL);
    if (display_rc522_info_queue == NULL)
    {
        char msg[] = "Failed to create display RC522 info queue\r\n";
        HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), 100);
        Error_Handler();
    }

    const osThreadAttr_t oled_task_attributes = {
        .name = OLED_TASK_THREAD_NAME,
        .priority = OLED_TASK_THREAD_PRIORITY,
        .stack_size = OLED_TASK_STACK_SIZE_BYTES
    };
    oled_task_handle = osThreadNew(OLED_Display_Task, NULL, &oled_task_attributes);
    if (oled_task_handle == NULL)
    {
        char msg[] = "Failed to create OLED display task\r\n";
        HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), 100);
        Error_Handler();
    }
}



/**
 * @brief Main loop for the OLED display RTOS task.
 *
 * This task continuously waits for RFID (RC522) data from the message queue and updates the OLED display
 * to show the current tag/card UID and access status. The corresponding status LED is also updated.
 *
 * The display layout:
 *   - Top line: Project name (defined by OLED_SHOW_PROJECT_NAME)
 *   - Middle line: Tag/Card UID or "Not Detected"
 *   - Bottom line: Status ("Success" or "Unsuccessful")
 *
 * @param argument Unused. Required by CMSIS-RTOS API for thread entry signature.
 *
 * @note This function should not be called directly. It is intended to be used as the thread entry point
 *       for the OLED display task, created via osThreadNew().
 *
 * @retval None. This function contains an infinite loop and does not return.
 */
static void OLED_Display_Task(void *argument)
{
    char rc522_display_str[32];
    RC522_Data_t rc522_data;
    OLED_Init();
    u8g2_t *u8g2 = OLED_GetDisplay();
    if (u8g2 == NULL)
    {
        char msg[] = "Failed to initialize OLED display\r\n";
        HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), 100);
        Error_Handler();
    }

    u8g2_ClearBuffer(u8g2);
    u8g2_ClearDisplay(u8g2);
    u8g2_SendBuffer(u8g2);
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    
    while (1) {

        // Block until new data arrives
        osStatus_t rc522Receive = osMessageQueueGet(display_rc522_info_queue, &rc522_data, NULL, osWaitForever);
        u8g2_ClearBuffer(u8g2);
        if (rc522_data.status == RC522_STATUS_SUCCESS) {
            snprintf(rc522_display_str, sizeof(rc522_display_str), "Tag/Card: %02X%02X%02X%02X", rc522_data.uid[0], rc522_data.uid[1], rc522_data.uid[2], rc522_data.uid[3]);
            u8g2_DrawStr(u8g2, 0, 28, rc522_display_str);
            u8g2_DrawStr(u8g2, 0, 46, "Status: Success");
            HAL_GPIO_WritePin(LED_PB14_GPIO_Port, LED_PB14_Pin, GPIO_PIN_SET);
        } else {
            u8g2_DrawStr(u8g2, 0, 28, "Tag/Card: Not Detected");
            u8g2_DrawStr(u8g2, 0, 46, "Status: Unsuccessful");
            HAL_GPIO_WritePin(LED_PB14_GPIO_Port, LED_PB14_Pin, GPIO_PIN_RESET);
        }
        // Show project name at the top line
        u8g2_DrawStr(u8g2, 0, 10, OLED_SHOW_PROJECT_NAME);
        u8g2_SendBuffer(u8g2);
        osDelay(100);
        
    }
}
