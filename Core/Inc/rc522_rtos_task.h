
/**
 * @file    rc522_rtos_task.h
 * @author  Ted Wang
 * @date    2025-08-26
 * @brief   RTOS task and data structure definitions for RC522 sensor (NUCLEO-F429ZI).
 *
 * @details
 * Provides type definitions, configuration macros, and API prototypes for the RC522 RTOS task
 * using CMSIS-RTOS v2. This header enables modular, maintainable, and well-documented code for
 * periodic RC522 sensor acquisition and inter-task communication.
 */


#ifndef RC522_RTOS_TASK_H
#define RC522_RTOS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"

/* Exported constants --------------------------------------------------------*/
/**
 * @def RC522_TASK_STACK_SIZE_BYTES
 * @brief Stack size (in bytes) for the RC522 RTOS task.
 */
#define RC522_TASK_STACK_SIZE_BYTES      (512 * 4)

/**
 * @def RC522_TASK_THREAD_NAME
 * @brief Name of the RC522 RTOS task (for debugging/RTOS awareness).
 */
#define RC522_TASK_THREAD_NAME           "RC522_Task"

/**
 * @def RC522_TASK_THREAD_PRIORITY
 * @brief Priority of the RC522 RTOS task.
 */
#define RC522_TASK_THREAD_PRIORITY       osPriorityAboveNormal

/**
 * @def RC522_QUEUE_SIZE
 * @brief Message queue size for RC522 data updates to display task.
 */
#define RC522_QUEUE_SIZE                 3

/* Exported types ------------------------------------------------------------*/
/**
 * @def RC522_STATUS_SUCCESS
 * @brief Status value indicating successful card detection.
 */
#define RC522_STATUS_SUCCESS      1

/**
 * @def RC522_STATUS_UNSUCCESSFUL
 * @brief Status value indicating unsuccessful card/tag detection.
 */
#define RC522_STATUS_UNSUCCESSFUL 0
/**
 * @brief RC522 sensor data structure.
 *
 * Holds the latest data readings from the RC522 sensor.
 */
typedef struct {
    uint8_t uid[10];      /**< UID of the detected RFID card */
    uint8_t uid_length;   /**< Length of the UID */
    uint8_t tagType[2];   /**< Card/tag type info from MFRC522_Request */
    uint8_t status;       /**< Status: success (1) or unsuccessful (0) */
} RC522_Data_t;

/* Exported variables --------------------------------------------------------*/
/**
 * @brief Global instance for latest RC522 sensor data.
 */
extern RC522_Data_t g_rc522_data;



/* Exported functions --------------------------------------------------------*/
/**
 * @brief  Initialize the RC522 RTOS task and its message queue.
 *
 * Configures and creates the RC522 acquisition task and its associated message queue
 * using CMSIS-RTOS v2 API. The task periodically samples the RC522 sensor and posts
 * results to the queue for other tasks (e.g., display). Call once during system initialization.
 */
void RC522_Task_Init(void);

#ifdef __cplusplus
}
#endif

#endif // RC522_RTOS_TASK_H
