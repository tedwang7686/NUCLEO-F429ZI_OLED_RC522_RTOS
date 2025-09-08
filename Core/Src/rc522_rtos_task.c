
/**
 * @file    rc522_rtos_task.c
 * @author  Ted Wang
 * @date    2025-08-26
 * @brief   RTOS task for MFRC522 RFID card/tag detection and status display (CMSIS-RTOS v2)
 *
 * @details
 * Implements a CMSIS-RTOS v2 task for real-time MFRC522 RFID card/tag detection.
 * The task reads the RC522 module, determines if a card/tag is detected, and sends the status (successful/unsuccessful) to a message queue for OLED display. Debug info is output via UART.
 */

/* Includes ------------------------------------------------------------------*/
#include "rc522_rtos_task.h"
#include "oled_rtos_task.h"
#include "rc522.h"
#include "main.h"
#include "oled_driver.h"
#include <string.h>
#include <stdio.h>


/**
 * @brief Global instance for latest RC522 sensor data.
 */
RC522_Data_t g_rc522_data;

/**
 * @brief RC522 RTOS task handle.
 */
static osThreadId_t rc522_task_handle;

/**
 * @brief UART3 handle for debug/error output (defined elsewhere).
 */
extern UART_HandleTypeDef huart3;

/**
 * @brief RC522 RTOS task main loop (thread entry point).
 * @param argument Unused (required by CMSIS-RTOS API)
 */
static void RC522_Task(void *argument);



/**
 * @brief  Initialize the RC522 RTOS task.
 *
 * This function creates the RC522 acquisition task. Call once during system initialization before the RTOS kernel starts.
 *
 * @note If task creation fails, outputs error via UART3 and calls Error_Handler().
 */
void RC522_Task_Init(void)
{
    const osThreadAttr_t rc522_task_attributes = {
        .name = RC522_TASK_THREAD_NAME,
        .priority = RC522_TASK_THREAD_PRIORITY,
        .stack_size = RC522_TASK_STACK_SIZE_BYTES
    };
    rc522_task_handle = osThreadNew(RC522_Task, NULL, &rc522_task_attributes);
    if (rc522_task_handle == NULL)
    {
        char msg[] = "Failed to create RC522 task\r\n";
        HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), 100);
        Error_Handler();
    }
}



/**
 * @brief Main loop for the RC522 RTOS acquisition task.
 *
 * This thread periodically reads the RC522 RFID sensor, sends card/tag data to the display queue,
 * and outputs debug information via UART. It supports Mifare S50/S70 cards (4-byte UID) and can be extended for other types.
 *
 * @param argument Unused (required by CMSIS-RTOS API).
 * @note This function runs as a CMSIS-RTOS v2 thread and should not be called directly.
 * @details
 * - Initializes the RC522 hardware and enters an infinite loop.
 * - Each cycle:
 *   - Requests card/tag presence and type via MFRC522_Request.
 *   - Performs anti-collision to read UID via MFRC522_Anticoll.
 *   - Populates RC522_Data_t structure with status, UID, and tag type.
 *   - Sends debug output via UART3.
 *   - Posts result to display_rc522_info_queue for UI/display.
 *   - Waits 3 seconds before next cycle.
 */
static void RC522_Task(void *argument)
{
    // Initialize the RC522 hardware before entering the main loop
    MFRC522_Init();

    while (1)
    {
        // Prepare a structure to hold the latest card/tag data
        RC522_Data_t rc522_data;
        memset(&rc522_data, 0, sizeof(rc522_data));

        // Request card/tag presence and type
        uint8_t tagType[2] = {0};
        uint8_t status = MFRC522_Request(PICC_REQIDL, tagType);

        // Output request result via UART for debugging
        char debug_msg[128];
        snprintf(debug_msg, sizeof(debug_msg), "MFRC522_Request status: %d, tagType: %02X%02X\r\n", status, tagType[0], tagType[1]);
        HAL_UART_Transmit(&huart3, (uint8_t *)debug_msg, strlen(debug_msg), 100);

        // Perform anti-collision to read UID
        uint8_t anticoll_status = MFRC522_Anticoll(rc522_data.uid);
        // Default UID length is 4 (Mifare S50/S70); extend for 7/10 bytes if needed
        rc522_data.uid_length = (anticoll_status == MI_OK) ? 4 : 0;
        rc522_data.tagType[0] = tagType[0];
        rc522_data.tagType[1] = tagType[1];

        // Output anti-collision result and UID via UART
        snprintf(debug_msg, sizeof(debug_msg), "MFRC522_Anticoll status: %d, UID: %02X%02X%02X%02X, UID_len: %d\r\n", anticoll_status, rc522_data.uid[0], rc522_data.uid[1], rc522_data.uid[2], rc522_data.uid[3], rc522_data.uid_length);
        HAL_UART_Transmit(&huart3, (uint8_t *)debug_msg, strlen(debug_msg), 100);

        // If both request and anti-collision succeed, report card/tag detected
        if (status == MI_OK && anticoll_status == MI_OK)
        {
            rc522_data.status = RC522_STATUS_SUCCESS;
            snprintf(debug_msg, sizeof(debug_msg), "Card/Tag detected! UID: %02X%02X%02X%02X, tagType: %02X%02X\r\n", rc522_data.uid[0], rc522_data.uid[1], rc522_data.uid[2], rc522_data.uid[3], rc522_data.tagType[0], rc522_data.tagType[1]);
            HAL_UART_Transmit(&huart3, (uint8_t *)debug_msg, strlen(debug_msg), 100);
        }
        else
        {
            rc522_data.status = RC522_STATUS_UNSUCCESSFUL;
            rc522_data.uid_length = 0;
            snprintf(debug_msg, sizeof(debug_msg), "No valid card/tag or UID not found\r\n");
            HAL_UART_Transmit(&huart3, (uint8_t *)debug_msg, strlen(debug_msg), 100);
        }

        // Send the result to the display queue for UI update
        osMessageQueuePut(display_rc522_info_queue, &rc522_data, 0, 0);

        // Wait 2 seconds before next acquisition cycle
        osDelay(2000);
    }
}
