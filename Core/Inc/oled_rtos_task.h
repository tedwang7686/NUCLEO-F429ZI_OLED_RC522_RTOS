
/**
 * @file    oled_rtos_task.h
 * @author  Ted Wang
 * @date    2025-08-01
 * @brief   RTOS task and configuration for OLED display (NUCLEO-F429ZI).
 *
 * @details
 * Provides type definitions, configuration macros, and API prototypes for the OLED RTOS task
 * using CMSIS-RTOS v2 and the u8g2 graphics library. The OLED displays the project name
 * (Access Control System) and the status of RFID tag/card detection (successful or not).
 */

#ifndef OLED_RTOS_TASK_H
#define OLED_RTOS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"
#include "u8g2.h"
#include "rc522_rtos_task.h"


/* Exported constants --------------------------------------------------------*/
/**
 * @def OLED_ANIMATION_DELAY_MS
 * @brief Animation frame delay for OLED display (milliseconds).
 */
#define OLED_ANIMATION_DELAY_MS      200

/**
 * @def OLED_SHOW_PROJECT_NAME
 * @brief Project name string displayed at the bottom of the OLED screen.
 */
#define OLED_SHOW_PROJECT_NAME         "Access Control System"

/**
 * @def OLED_TASK_STACK_SIZE_BYTES
 * @brief Stack size (in bytes) for the OLED RTOS task.
 */
#define OLED_TASK_STACK_SIZE_BYTES   (512 * 4)

/**
 * @def OLED_TASK_THREAD_NAME
 * @brief Name of the OLED RTOS task (for debugging/RTOS awareness).
 */
#define OLED_TASK_THREAD_NAME        "OLED_Task"

/**
 * @def OLED_TASK_THREAD_PRIORITY
 * @brief Priority of the OLED RTOS task.
 */
#define OLED_TASK_THREAD_PRIORITY    osPriorityNormal

/**
 * @def RC522_QUEUE_SIZE
 * @brief Message queue size for RC522 data updates (shared with OLED task).
 */
#define RC522_QUEUE_SIZE 3

/* Exported variables --------------------------------------------------------*/
/**
 * @brief Message queue handle for RC522 data updates (shared by RC522 and OLED tasks).
 */
extern osMessageQueueId_t display_rc522_info_queue;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Initialize the OLED display RTOS task and message queue.
 *
 * Configures and creates the OLED display task and its associated message queue
 * using CMSIS-RTOS v2 API. The task displays the project name and the status of RFID tag/card detection.
 * Status will show "successful" if a tag/card is detected, otherwise show "unsuccessful" or "no card/tag".
 * Call once during system initialization.
 */
void OLED_Task_Init(void);


#ifdef __cplusplus
}
#endif

#endif // OLED_RTOS_TASK_H