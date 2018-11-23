/* ---  STC International Limited ---------------- */
/* ---  ºê¾§¿?¼¼ ?¦???½  ?è¼? 2004/9/11   V1.0 --- */
/* ---  ºê¾§¿?¼¼ ?¦???½  ?è¼? 2004/12/4   V2.0 --- */
/* ---  RD+/RC?µ??   Flash IAP/ISP Operation ----- */
/* ---  STC89C54RD+, STC89C58RD+,              --- */
/* ---  STC89LE54RD+,STC89LE58RD+,              -- */
/* ---  STC89C51RC,  STC89C52RC,             ----- */
/* ---  STC89LE51RC, STC89LE52RC,             ---- */
/* ---  Mobile: 13922805190 ---------------------- */
/* ---  Fax: 0755-82944243 ----------------------- */
/* ---  Tel: 0755-82908285 ----------------------- */
/* ---  Web  : www.mcu-memory.com ---------------- */
/* ---  ¸??»?ø??2004/12/3µ??¸?ý,???ù?·???¾¸??» --- */

#include <mcs51/at89x52.h>

#include "common.h"
#include "eeprom.h"

/*        ???ö???â¹¦??¼?´æ?÷¶¨?å        */
__sfr __at(0xe2) ISP_DATA;
__sfr __at(0xe3) ISP_ADDRH;
__sfr __at(0xe4) ISP_ADDRL;
__sfr __at(0xe5) ISP_CMD;
__sfr __at(0xe6) ISP_TRIG;
__sfr __at(0xe7) ISP_CONTR;

/* ¶¨?å?ü?î */
#define READ_AP_and_Data_Memory_Command 0x01
#define PROGRAM_AP_and_Data_Memory_Command 0x02
#define SECTOR_ERASE_AP_and_Data_Memory_Command 0x03

typedef unsigned char INT8U; /* 8 bit ??·ûº??û??  */
typedef unsigned int INT16U; /* 16 bit ??·ûº??û?? */
//#define		DELAY_CONST         60000

/* ¶¨?å³£?¿ */
#define ERROR 0
#define OK 1

/* ¶¨?åFlash ²??÷µ?´ý?±¼ä */
#define MCU_CLOCK_20MHz
//#define         MCU_CLOCK_20MHz
//#define        MCU_CLOCK_10MHz
//#define        MCU_CLOCK_5MHz
#ifdef MCU_CLOCK_40MHz
#define WAIT_TIME 0x00
#endif
#ifdef MCU_CLOCK_20MHz
#define WAIT_TIME 0x01
#endif
#ifdef MCU_CLOCK_10MHz
#define WAIT_TIME 0x02
#endif
#ifdef MCU_CLOCK_5MHz
#define WAIT_TIME 0x03
#endif

/* µ÷??¿????î */
//#define         DEBUG_STC89C_LE58RD

#define DEBUG_STC89C_LE52RC

//#define USED_BYTE_QTY_IN_ONE_SECTOR                1
//#define USED_BYTE_QTY_IN_ONE_SECTOR                2
//#define USED_BYTE_QTY_IN_ONE_SECTOR                4
//#define USED_BYTE_QTY_IN_ONE_SECTOR                8
//#define USED_BYTE_QTY_IN_ONE_SECTOR                16
//#define USED_BYTE_QTY_IN_ONE_SECTOR                32
//#define USED_BYTE_QTY_IN_ONE_SECTOR                64
#define USED_BYTE_QTY_IN_ONE_SECTOR 128
//#define USED_BYTE_QTY_IN_ONE_SECTOR                256
//#define USED_BYTE_QTY_IN_ONE_SECTOR                512

#ifdef DEBUG_STC89C_LE58RD //STC89C58RD+,  89LE58RD+
#define DEBUG_AP_Memory_Begin_Sector_addr 0x0000
#define DEBUG_AP_Memory_End_Sector_addr 0x7e00
#define DEBUG_AP_Memory_End_Byte_addr 0x7fff

#define DEBUG_Data_Memory_Begin_Sector_addr 0x8000
#endif
#ifdef DEBUG_STC89C_LE52RC //STC89C52RC,        89LE52RC
#define DEBUG_AP_Memory_Begin_Sector_addr 0x0000
#define DEBUG_AP_Memory_End_Sector_addr 0x1e00
#define DEBUG_AP_Memory_End_Byte_addr 0x1fff

#define DEBUG_Data_Memory_Begin_Sector_addr 0x2000
#endif

#define CALIBRATION_VALUES_START_ADDR DEBUG_Data_Memory_Begin_Sector_addr

/* ´ò¿ª ISP,IAP ¹¦?? */
void ISP_IAP_enable(void)
{
	EA = 0;						  /* ¹???¶? */
	ISP_CONTR = ISP_CONTR & 0x18; /* 0001,1000 */
	ISP_CONTR = ISP_CONTR | WAIT_TIME;
	ISP_CONTR = ISP_CONTR | 0x80; /* 1000,0000 */
}

/* ¹?±? ISP,IAP ¹¦?? */
void ISP_IAP_disable(void)
{
	ISP_CONTR = ISP_CONTR & 0x7f; /* 0111,1111 */
	ISP_TRIG = 0x00;
	EA = 1; /* ¿ª??¶? */
}

/* ??½?¶? */
INT8U byte_read(INT16U byte_addr)
{
	ISP_ADDRH = (INT8U)(byte_addr >> 8);
	ISP_ADDRL = (INT8U)(byte_addr & 0x00ff);

	ISP_CMD = ISP_CMD & 0xf8;							 /* 1111,1000 */
	ISP_CMD = ISP_CMD | READ_AP_and_Data_Memory_Command; /* 0000,0001 */

	ISP_IAP_enable();

	ISP_TRIG = 0x46;
	ISP_TRIG = 0xb9;
	_nop_();

	ISP_IAP_disable();
	return (ISP_DATA);
}

/* ???ø²?³ý */
INT8U sector_erase(INT16U sector_addr)
{
	INT16U get_sector_addr = 0;
	get_sector_addr = (sector_addr & 0xfe00); /* 1111,1110,0000,0000; ?¡???øµ??· */
	ISP_ADDRH = (INT8U)(get_sector_addr >> 8);
	ISP_ADDRL = 0x00;

	ISP_CMD = ISP_CMD & 0xf8;									 /* 1111,1000 */
	ISP_CMD = ISP_CMD | SECTOR_ERASE_AP_and_Data_Memory_Command; /* 0000,0011 */

	ISP_IAP_enable();
	ISP_TRIG = 0x46; /* ´¥·¢ISP_IAP?ü?î */
	ISP_TRIG = 0xb9; /* ´¥·¢ISP_IAP?ü?î */
	_nop_();

	ISP_IAP_disable();
	return OK;
}

/* ?´?ý¾?½ø ?ý¾?Flash´æ´¢?÷, ?»???¬?»¸ö???ø???´£¬²»±£?ô?­???ý¾?	*/
/* begin_addr,±»?´?ý¾?Flash¿ª?¼µ??·£»counter,?¬?ø?´¶???¸ö??½?£» array[]£¬?ý¾??´?´	*/
INT8U sequential_write_flash_in_one_sector(INT16U begin_addr, INT16U counter, const INT8U array[])
{
	INT16U i = 0;
	INT16U in_sector_begin_addr = 0;
	INT16U sector_addr = 0;

	/* ????·ñ?????§·¶?§,´?º¯?ý²»???í¿ç???ø²??÷ */
	if (counter > USED_BYTE_QTY_IN_ONE_SECTOR)
		return ERROR;
	in_sector_begin_addr = begin_addr & 0x01ff; /* 0000,0001,1111,1111 */
	if ((in_sector_begin_addr + counter) > USED_BYTE_QTY_IN_ONE_SECTOR)
		return ERROR;

	/* ²?³ý ?ª??¸?/?´?ë µ????ø */
	sector_addr = (begin_addr & 0xfe00); /* 1111,1110,0000,0000; ?¡???øµ??· */
	ISP_ADDRH = (INT8U)(sector_addr >> 8);
	ISP_ADDRL = 0x00;
	ISP_CMD = ISP_CMD & 0xf8;									 /* 1111,1000 */
	ISP_CMD = ISP_CMD | SECTOR_ERASE_AP_and_Data_Memory_Command; /* 0000,0011 */

	ISP_IAP_enable();
	ISP_TRIG = 0x46; /* ´¥·¢ISP_IAP?ü?î */
	ISP_TRIG = 0xb9; /* ´¥·¢ISP_IAP?ü?î */
	_nop_();

	for (i = 0; i < counter; i++)
	{
		/* ?´?»¸ö??½? */
		ISP_ADDRH = (INT8U)(begin_addr >> 8);
		ISP_ADDRL = (INT8U)(begin_addr & 0x00ff);
		ISP_DATA = array[i];
		ISP_CMD = ISP_CMD & 0xf8;								/* 1111,1000 */
		ISP_CMD = ISP_CMD | PROGRAM_AP_and_Data_Memory_Command; /* 0000,0010 */

		ISP_TRIG = 0x46; /* ´¥·¢ISP_IAP?ü?î */
		ISP_TRIG = 0xb9; /* ´¥·¢ISP_IAP?ü?î */
		_nop_();

		/* ¶?»??´ */
		ISP_DATA = 0x00;

		ISP_CMD = ISP_CMD & 0xf8;							 /* 1111,1000 */
		ISP_CMD = ISP_CMD | READ_AP_and_Data_Memory_Command; /* 0000,0001 */

		ISP_TRIG = 0x46; /* ´¥·¢ISP_IAP?ü?î */
		ISP_TRIG = 0xb9; /* ´¥·¢ISP_IAP?ü?î */
		_nop_();

		/*  ±?½?¶?´í */
		if (ISP_DATA != array[i])
		{
			ISP_IAP_disable();
			return ERROR;
		}
		begin_addr++;
	}
	ISP_IAP_disable();
	return OK;
}

void ReadCalibrationValues(struct calibration_values_type *values)
{
	INT8U *pBuf = (INT8U *)(void *)values;
	INT8U byte;
	for (byte = 0; byte < sizeof(struct calibration_values_type); ++byte, ++pBuf)
	{
		*pBuf = byte_read(CALIBRATION_VALUES_START_ADDR + byte);
	}
}

void WriteCalibrationValues(const struct calibration_values_type *values)
{
	sector_erase(CALIBRATION_VALUES_START_ADDR);
	sequential_write_flash_in_one_sector(CALIBRATION_VALUES_START_ADDR, sizeof(struct calibration_values_type), (const INT8U *)(const void *)values);
}

void EraseCalibrationValues()
{
	sector_erase(CALIBRATION_VALUES_START_ADDR);
}
