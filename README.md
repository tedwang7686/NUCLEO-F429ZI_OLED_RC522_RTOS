
# STM32 NUCLEO-F429ZI RTOS RFID OLED Display Project


This project demonstrates how to use the STM32 NUCLEO-F429ZI board with an MFRC522 RFID reader and a 128x64 OLED (I2C, u8g2 driver), leveraging CMSIS-RTOS v2 (FreeRTOS) for professional multitasking display. The OLED shows the project name "Access Control System" and the status of RFID tag/card detection (successful or unsuccessful), and will also display the detected RFID card/tag UID. The codebase is modular, supporting real-time RFID card/tag detection with high maintainability and extensibility.



## Features
- **Hardware**: STM32 NUCLEO-F429ZI, MFRC522 RFID module (SPI: SCK→PD3, MISO→PC2, MOSI→PC3, NSS/CS→PB8, RST→PB9), OLED (I2C: SCL→PF1, SDA→PF0)
- **Software**: STM32 HAL, FreeRTOS (CMSIS-RTOS v2), u8g2 graphics library
- **Functionality**:

   - OLED displays the project name "Access Control System" and RFID tag/card detection status
   - Detected RFID card/tag UID will be displayed on the OLED
   - Status will show "successful" if a tag/card is detected, otherwise show "unsuccessful" or "no card/tag"
   - RFID data is periodically acquired by an RTOS task and sent via message queue
   - Professional Doxygen comments, clear structure, easy to maintain and extend
   - UART3 outputs debug and error messages



## Directory Structure
```
NUCLEO-F429ZI_OLED_RC522_RTOS/
├── Core/
│   ├── Inc/         # Header files (main.h, oled_rtos_task.h, rc522_rtos_task.h ...)
│   └── Src/         # Source files (main.c, oled_rtos_task.c, rc522_rtos_task.c ...)
├── Hardware/
│   ├── oled/        # OLED driver
│   ├── rc522/       # MFRC522 RFID driver
│   └── u8g2/        # u8g2 graphics library
├── Drivers/         # HAL, CMSIS, etc.
├── MDK-ARM/         # Keil project files
├── Middlewares/     # Third-party middleware (e.g., FreeRTOS)
├── README.md        # This documentation
└── LICENSE          # License file
```



## Quick Start
1. **Hardware Connection**:
   - OLED SCL → PF1, SDA → PF0, VCC → 5V, GND → GND
   - MFRC522 RFID: SCK → PD3, MISO → PC2, MOSI → PC3, NSS/CS → PB8, RST → PB9, VCC → 3.3V, GND → GND
2. **Development Environment**:
   - Keil uVision
   - Use STM32CubeMX to verify I2C2/SPI2/FreeRTOS configuration
3. **Build & Flash**:
   - Import the project into your IDE, build, and flash to NUCLEO-F429ZI
4. **Display Verification**:
   - Default shows RFID card/tag info (including UID) on OLED



## Main Code Structure
- `rc522_rtos_task.c/h`: MFRC522 RFID acquisition task, card/tag queue
- `oled_rtos_task.c/h`: OLED display task, project name and info display
- `oled_driver.c/h`: OLED initialization and u8g2 interface
- `Hardware/oled/`: OLED low-level driver
- `Hardware/rc522/`: MFRC522 RFID low-level driver
- `Hardware/u8g2/`: u8g2 graphics library




## Advanced Features
- **RFID Card/Tag Detection**: Real-time detection and display of card/tag UID and status (successful/unsuccessful)
- **Doxygen Documentation**: All core code is documented with professional English Doxygen comments
- **Extensible**: Easily add new display modes or sensors if needed
- **Error Handling**: UART3 outputs error messages; queue/task creation failure triggers Error_Handler



## License
This project is licensed under the MIT License. See LICENSE for details.
The u8g2 graphics library is licensed under Apache 2.0; please retain its LICENSE file.



## Acknowledgments
- [u8g2 Library](https://github.com/olikraus/u8g2)
- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
- [FreeRTOS](https://www.freertos.org)