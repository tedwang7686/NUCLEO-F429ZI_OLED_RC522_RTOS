
/**
 * @file RC522.c
 * @author Ted Wang
 * @date 2025-09-01
 * @brief MFRC522 RFID driver implementation for STM32 (SPI, register access, card communication).
 *
 * This file provides the implementation of the MFRC522 RFID reader driver for the NUCLEO-F429ZI board, including:
 *   - STM32-specific SPI byte transfer and register access functions
 *   - Initialization and control of the MFRC522 module
 *   - Card communication routines (request, anticollision, authentication, read/write, halt)
 *
 * The driver is designed for use with the CMSIS HAL and supports ISO14443A cards.
 */

#include "RC522.h"
#include <stdint.h>

/**
 * @brief Transfers a byte via SPI to the RC522 and receives the response.
 *
 * This function is used by Write_MFRC522 and Read_MFRC522 to communicate with the RC522 module.
 *
 * @param data The byte to be transmitted to the RC522.
 * @return The byte received from the RC522.
 */
uint8_t RC522_SPI_Transfer(uchar data)
{
	uchar rx_data;
	HAL_SPI_TransmitReceive(HSPI_INSTANCE,&data,&rx_data,1,100);

	return rx_data;
}

/**
 * @brief Writes a byte to a specific MFRC522 register.
 *
 * Sets the chip select (CS) low, sends the register address and value via SPI, then sets CS high.
 *
 * @param addr Register address to write to.
 * @param val Value to write to the register.
 */
void Write_MFRC522(uchar addr, uchar val)
{
	/* CS LOW */
	HAL_GPIO_WritePin(MFRC522_CS_PORT,MFRC522_CS_PIN,GPIO_PIN_RESET);

	  // even though we are calling transfer frame once, we are really sending
	  // two 8-bit frames smooshed together-- sending two 8 bit frames back to back
	  // results in a spike in the select line which will jack with transactions
	  // - top 8 bits are the address. Per the spec, we shift the address left
	  //   1 bit, clear the LSb, and clear the MSb to indicate a write
	  // - bottom 8 bits are the data bits being sent for that address, we send them
	RC522_SPI_Transfer((addr<<1)&0x7E);	
	RC522_SPI_Transfer(val);
	
	/* CS HIGH */
	HAL_GPIO_WritePin(MFRC522_CS_PORT,MFRC522_CS_PIN,GPIO_PIN_SET);
}

/**
 * @brief Reads a byte from a specific MFRC522 register.
 *
 * Sets the chip select (CS) low, sends the register address via SPI, reads the value, then sets CS high.
 *
 * @param addr Register address to read from.
 * @return Value read from the register.
 */
uchar Read_MFRC522(uchar addr)
{
	uchar val;

	/* CS LOW */
	HAL_GPIO_WritePin(MFRC522_CS_PORT,MFRC522_CS_PIN,GPIO_PIN_RESET);

	  // even though we are calling transfer frame once, we are really sending
	  // two 8-bit frames smooshed together-- sending two 8 bit frames back to back
	  // results in a spike in the select line which will jack with transactions
	  // - top 8 bits are the address. Per the spec, we shift the address left
	  //   1 bit, clear the LSb, and set the MSb to indicate a read
	  // - bottom 8 bits are all 0s on a read per 8.1.2.1 Table 6
	RC522_SPI_Transfer(((addr<<1)&0x7E) | 0x80);	
	val = RC522_SPI_Transfer(0x00);
	
	/* CS HIGH */
	HAL_GPIO_WritePin(MFRC522_CS_PORT,MFRC522_CS_PIN,GPIO_PIN_SET);
	
	return val;	
	
}

/**
 * @brief Sets specific bits in a MFRC522 register.
 *
 * Reads the register, ORs the value with the mask, and writes it back.
 *
 * @param reg Register address.
 * @param mask Bit mask to set.
 */
void SetBitMask(uchar reg, uchar mask)  
{
    uchar tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp | mask);  // set bit mask
}

/**
 * @brief Clears specific bits in a MFRC522 register.
 *
 * Reads the register, ANDs the value with the inverse mask, and writes it back.
 *
 * @param reg Register address.
 * @param mask Bit mask to clear.
 */
void ClearBitMask(uchar reg, uchar mask)  
{
    uchar tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp & (~mask));  // clear bit mask
} 

/**
 * @brief Turns on the RC522 antenna.
 *
 * Reads the TxControlReg and sets the antenna bit mask to enable transmission.
 */
void AntennaOn(void)
{

	Read_MFRC522(TxControlReg);
	SetBitMask(TxControlReg, 0x03);
}

/**
 * @brief Turns off the RC522 antenna.
 *
 * Clears the antenna bit mask in TxControlReg to disable transmission.
 */
void AntennaOff(void)
{
	ClearBitMask(TxControlReg, 0x03);
}

/**
 * @brief Resets the MFRC522 module.
 *
 * Writes the reset command to the CommandReg register.
 */
void MFRC522_Reset(void)
{
    Write_MFRC522(CommandReg, PCD_RESETPHASE);
}

/**
 * @brief Initializes the MFRC522 module for operation.
 *
 * Sets CS and RST pins, resets the module, configures timer and modulation registers, and enables the antenna.
 */
void MFRC522_Init(void)
{
	HAL_GPIO_WritePin(MFRC522_CS_PORT,MFRC522_CS_PIN,GPIO_PIN_SET);
	HAL_GPIO_WritePin(MFRC522_RST_PORT,MFRC522_RST_PIN,GPIO_PIN_SET);
	MFRC522_Reset();
	 	
	//Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
	Write_MFRC522(TModeReg, 0x8D);		//Tauto=1; f(Timer) = 6.78MHz/TPreScaler
	Write_MFRC522(TPrescalerReg, 0x3E);	//TModeReg[3..0] + TPrescalerReg
	Write_MFRC522(TReloadRegL, 30);           
	Write_MFRC522(TReloadRegH, 0);
	
	Write_MFRC522(TxAutoReg, 0x40);		// force 100% ASK modulation
	Write_MFRC522(ModeReg, 0x3D);		// CRC Initial value 0x6363

	AntennaOn();
}

/**
 * @brief Communicates with an ISO14443 card via the MFRC522.
 *
 * Sends a command and data to the card, receives the response, and returns the status.
 *
 * @param command MFRC522 command word.
 * @param sendData Data to send to the card.
 * @param sendLen Length of data to send.
 * @param backData Buffer to store received data from the card.
 * @param backLen Pointer to store the number of bits received.
 * @return MI_OK if successful, otherwise error code.
 */
uchar MFRC522_ToCard(uchar command, uchar *sendData, uchar sendLen, uchar *backData, uint *backLen)
{
    uchar status = MI_ERR;
    uchar irqEn = 0x00;
    uchar waitIRq = 0x00;
    uchar lastBits;
    uchar n;
    uint i;

    switch (command)
    {
        case PCD_AUTHENT:		// Certification cards close
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE:	// Transmit FIFO data
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
			break;
    }
   
    Write_MFRC522(CommIEnReg, irqEn|0x80);	// Interrupt request
    ClearBitMask(CommIrqReg, 0x80);			// Clear all interrupt request bit
    SetBitMask(FIFOLevelReg, 0x80);			// FlushBuffer=1, FIFO Initialization
    
	Write_MFRC522(CommandReg, PCD_IDLE);	// NO action; Cancel the current command

	// Writing data to the FIFO
    for (i=0; i<sendLen; i++)
    {   
		Write_MFRC522(FIFODataReg, sendData[i]);    
	}

    // Execute the command
	Write_MFRC522(CommandReg, command);
    if (command == PCD_TRANSCEIVE)
    {    
		SetBitMask(BitFramingReg, 0x80);		// StartSend=1,transmission of data starts
	}   
    
    // Waiting to receive data to complete
	i = 2000;	// i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms
    do 
    {
		//CommIrqReg[7..0]
		//Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
        n = Read_MFRC522(CommIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitIRq));

    ClearBitMask(BitFramingReg, 0x80);			//StartSend=0
	
    if (i != 0)
    {    
        if(!(Read_MFRC522(ErrorReg) & 0x1B))	//BufferOvfl Collerr CRCErr ProtecolErr
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {   
				status = MI_NOTAGERR;
			}

            if (command == PCD_TRANSCEIVE)
            {
               	n = Read_MFRC522(FIFOLevelReg);
              	lastBits = Read_MFRC522(ControlReg) & 0x07;
                if (lastBits)
                {   
					*backLen = (n-1)*8 + lastBits;   
				}
                else
                {   
					*backLen = n*8;   
				}

                if (n == 0)
                {   
					n = 1;    
				}
                if (n > MAX_LEN)
                {   
					n = MAX_LEN;   
				}
				
                // Reading the received data in FIFO
                for (i=0; i<n; i++)
                {   
					backData[i] = Read_MFRC522(FIFODataReg);    
				}
            }
        }
        else
        {   
			status = MI_ERR;  
		}
        
    }
	
    //SetBitMask(ControlReg,0x80);           //timer stops
    //Write_MFRC522(CommandReg, PCD_IDLE); 

    return status;
}

/**
 * @brief Requests card presence and reads the card type.
 *
 * Sends a request command to detect a card and returns the card type if found.
 *
 * @param reqMode Request mode (PICC_REQIDL or PICC_REQALL).
 * @param TagType Pointer to store the returned card type.
 *        - 0x4400 = Mifare_UltraLight
 *        - 0x0400 = Mifare_One(S50)
 *        - 0x0200 = Mifare_One(S70)
 *        - 0x0800 = Mifare_Pro(X)
 *        - 0x4403 = Mifare_DESFire
 * @return MI_OK if successful, otherwise error code.
 */
uchar MFRC522_Request(uchar reqMode, uchar *TagType)
{
	uchar status;  
	uint backBits;			 // The received data bits

	Write_MFRC522(BitFramingReg, 0x07);		//TxLastBists = BitFramingReg[2..0]
	
	TagType[0] = reqMode;
	status = MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10))
	{    
		status = MI_ERR;
	}
   
	return status;
}

/**
 * @brief Performs anti-collision detection and reads the card serial number.
 *
 * Sends the anti-collision command and retrieves the card's 4-byte serial number.
 *
 * @param serNum Pointer to buffer for the returned serial number (5 bytes, last is checksum).
 * @return MI_OK if successful, otherwise error code.
 */
uchar MFRC522_Anticoll(uchar *serNum)
{
    uchar status;
    uchar i;
	uchar serNumCheck=0;
    uint unLen;
    
	Write_MFRC522(BitFramingReg, 0x00);		//TxLastBists = BitFramingReg[2..0]
 
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
	{
    	 //Check card serial number
		for (i=0; i<4; i++)
		{   
		 	serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i])
		{   
			status = MI_ERR;    
		}
    }

    return status;
} 

/**
 * @brief Calculates CRC using the MFRC522 hardware.
 *
 * Writes data to FIFO, triggers CRC calculation, and reads the result.
 *
 * @param pIndata Pointer to input data for CRC calculation.
 * @param len Length of input data.
 * @param pOutData Pointer to buffer for CRC result (2 bytes).
 */
void CalulateCRC(uchar *pIndata, uchar len, uchar *pOutData)
{
    uchar i, n;

    ClearBitMask(DivIrqReg, 0x04);			//CRCIrq = 0
    SetBitMask(FIFOLevelReg, 0x80);			//Clear the FIFO pointer

    //Writing data to the FIFO
    for (i=0; i<len; i++)
    {   
		Write_MFRC522(FIFODataReg, *(pIndata+i));   
	}
    Write_MFRC522(CommandReg, PCD_CALCCRC);

    //Wait CRC calculation is complete
    i = 0xFF;
    do 
    {
        n = Read_MFRC522(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));			//CRCIrq = 1

    //Read CRC calculation result
    pOutData[0] = Read_MFRC522(CRCResultRegL);
    pOutData[1] = Read_MFRC522(CRCResultRegH);
}

/**
 * @brief Selects a card and reads its memory capacity.
 *
 * Sends the select command with the card serial number and returns the card's memory size.
 *
 * @param serNum Pointer to the card serial number.
 * @return Card memory size if successful, otherwise 0.
 */
uchar MFRC522_SelectTag(uchar *serNum)
{
	uchar i;
	uchar status;
	uchar size;
	uint recvBits;
	uchar buffer[9]; 

	//ClearBitMask(Status2Reg, 0x08);			//MFCrypto1On=0

    buffer[0] = PICC_SElECTTAG;
    buffer[1] = 0x70;
    for (i=0; i<5; i++)
    {
    	buffer[i+2] = *(serNum+i);
    }
	CalulateCRC(buffer, 7, &buffer[7]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
    
    if ((status == MI_OK) && (recvBits == 0x18))
    {   
		size = buffer[0]; 
	}
    else
    {   
		size = 0;    
	}

    return size;
}

/**
 * @brief Authenticates a card block using a key.
 *
 * Sends authentication command with key and card serial number to verify access to a block.
 *
 * @param authMode Authentication mode (0x60 = Key A, 0x61 = Key B).
 * @param BlockAddr Block address to authenticate.
 * @param Sectorkey Pointer to 6-byte sector key.
 * @param serNum Pointer to 4-byte card serial number.
 * @return MI_OK if successful, otherwise error code.
 */
uchar MFRC522_Auth(uchar authMode, uchar BlockAddr, uchar *Sectorkey, uchar *serNum)
{
    uchar status;
    uint recvBits;
    uchar i;
	uchar buff[12]; 

	//Verify the command block address + sector + password + card serial number
    buff[0] = authMode;
    buff[1] = BlockAddr;
    for (i=0; i<6; i++)
    {    
		buff[i+2] = *(Sectorkey+i);   
	}
    for (i=0; i<4; i++)
    {    
		buff[i+8] = *(serNum+i);   
	}
    status = MFRC522_ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);

    if ((status != MI_OK) || (!(Read_MFRC522(Status2Reg) & 0x08)))
    {   
		status = MI_ERR;   
	}
    
    return status;
}

/**
 * @brief Reads data from a card block.
 *
 * Sends the read command to the card and retrieves block data.
 *
 * @param blockAddr Block address to read.
 * @param recvData Pointer to buffer for received block data.
 * @return MI_OK if successful, otherwise error code.
 */
uchar MFRC522_Read(uchar blockAddr, uchar *recvData)
{
    uchar status;
    uint unLen;

    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    CalulateCRC(recvData,2, &recvData[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

    if ((status != MI_OK) || (unLen != 0x90))
    {
        status = MI_ERR;
    }
    
    return status;
}

/**
 * @brief Writes data to a card block.
 *
 * Sends the write command and 16 bytes of data to the card block.
 *
 * @param blockAddr Block address to write.
 * @param writeData Pointer to 16-byte data to write.
 * @return MI_OK if successful, otherwise error code.
 */
uchar MFRC522_Write(uchar blockAddr, uchar *writeData)
{
    uchar status;
    uint recvBits;
    uchar i;
	uchar buff[18]; 
    
    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    CalulateCRC(buff, 2, &buff[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    {   
		status = MI_ERR;   
	}
        
    if (status == MI_OK)
    {
        for (i=0; i<16; i++)		//Data to the FIFO write 16Byte
        {    
        	buff[i] = *(writeData+i);   
        }
        CalulateCRC(buff, 16, &buff[16]);
        status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);
        
		if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
        {   
			status = MI_ERR;   
		}
    }
    
    return status;
}

/**
 * @brief Sends the halt command to the card to enter hibernation.
 *
 * Puts the card into a halt state until next activation.
 */
void MFRC522_Halt(void)
{
	uint unLen;
	uchar buff[4]; 

	buff[0] = PICC_HALT;
	buff[1] = 0;
	CalulateCRC(buff, 2, &buff[2]);
 
	MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff,&unLen);
}