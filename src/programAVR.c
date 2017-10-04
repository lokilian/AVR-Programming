
#include "AVRbins.h"

#define SCLK      PORTBbits.RB14 // change port and bit to your SCLK pin
#define MOSI      PORTBbits.RB13 // change port and bit to your MOSI pin    
#define MISO      PORTBbits.RB11 // change port and bit to your MISO pin 

#define NSECS_TO_TICK(T) ((1950 * (T)) / 1000)
#define MSECS_TICK(s) (1950 * (s))	//NOTE these were made using a digital analyser to time the delay
									//it may not be desirable for your PIC and so you may want to change it

#define RST(state) do { if ((state)) LATBCLR = BIT_9; else LATBSET = BIT_9;} while (0)// change port and bit to your RESET pin 

//note this times may change chip to chip so check data sheet
#define twd_flash 4 //on datasheet twdflash minimum delay is 2.6ms
#define twd_eeprom 5 //on datasheet twdeeprom minimum delay is 3.6ms
#define twd_erase 13 //on datasheet twderase minimum delay is 10.5ms
#define twd_fuse 6 //on datasheet twdfuse minimum delay is 4.5ms

static uint8_t response[4];


static void Delayms(int d)
{
  uint32_t i;
  for (i = MSECS_TICK(d);i!=0; i--);
}
Delayns(int d)
{
  uint32_t i;
  for (i = NSECS_TICK(d);i!=0; i--);
}


static void SPI_Init(void)
{
    
    SPI1CONbits.ON = 0;         // Turn off SPI module

    SPI1STATCLR = 0x40;    // Clear overflow flag 
    /* SPI1 configuration */
    SPI1CON2bits.AUDEN=0;
    SPI1CONbits.FRMEN = 0;      // Framed SPI support is disabled
    SPI1CONbits.SIDL = 0;       // Continue operation in IDLE mode
    SPI1CONbits.DISSDO = 0;     // SDO1 pin is controlled by the module
    SPI1CONbits.MODE16 = 0;     // 8 bit mode
    SPI1CONbits.MODE32 = 0;
    SPI1CONbits.ENHBUF=1;
    SPI1CONbits.SSEN = 0;       // Not in slave mode
    SPI1CONbits.MSTEN = 1;      // Master mode

	SPI1CONbits.SMP = 1;        // Input data sampled at the end of data output time 
	SPI1CONbits.CKP = 0;        // Idle state for clock is low, active state is high
	SPI1CONbits.CKE = 1;        // Output data changes on transition from idle to active
	SPI1CONbits.ON = 1;         // Turn module on

}
    



static uint8_t writeread_byte(uint8_t wv)//write Value
{
    uint8_t rv=0;//read value
    for ( i = 7; i >= 0; i--)
    {
      MOSI = (wv >> i) & 0x1;//set the latch to the bit value
      rv = (rv << 1) | MISOPORT;// shift in MISO value
      SCLK = 1; //set clock low
      SCLK = 0;
    }
    
  return rv;
}

static uint8_t writeread_byte_alt(uint8_t wv)//write Value
{//for use with SPI peripheral. ( requires SPI_Init(); )
    uint8_t rv=0;//read value
    SPI1BUF=wv;
    rv=SPI1BUF;
  return rv;
}
static void reset_response(void)
{
    response[0] = 0;
    response[1] = 0;
    response[2] = 0;
    response[3] = 0;
}

static uint8_t mux_writecmd(uint8_t A,uint8_t B,uint8_t C,uint8_t D)
{
	reset_response();
    response[0] = mux_writeread_byte(A);//write each byte and read the returned byte
    response[1] = mux_writeread_byte(B);
    response[2] = mux_writeread_byte(C);
    response[3] = mux_writeread_byte(D);

    if (A == 0xAC)
    {
      if      (B == 0x80) Delayms(twd_erase); //AC80=erase
      else if (B == 0x53) Delayms(twd_flash);  //AC53=programming mode
      else  Delayms(twd_fuse);

    }
	else if (A == 0x40);//write to memory page lowbyte
	else if (A == 0x48);//write to memory page highbyte
    else if (A == 0x4C)Delayms(twd_eeprom); //
    else if (A == 0xC0)Delayms(twd_eeprom);//C0 is write eeprom memory
    else Delayms(twd_flash);
   

  return response[3];
}

static void  check_mux_ID(void)
{
    mux_writecmd(0x30,0x00,0x00,0x00);//program command
}

static void mux_erase(void)
{
    mux_writecmd(0xAC,0x80,0x00,0x00);//erase command
    MUXRST(0);
    Delayns(40);
    MUXRST(1);
    Delayns(40);
}

static void mux_programmode(void)
{
    mux_writecmd(0xAC,0x53,0x00,0x00);//program command
}

void program(void)
{
	SPI_Init();
	uint8_t TIMEOUT=0;
	RST(1);// PUT RESET LOW (AVR has a !RESET)
	progRST:;
	programmode();
	check_mux_ID();
	if(response[3]==0x1E || response[3]==0x00)
	{// avr responded and is now in program mode
		erase();
		//assuming erase functioned correctly
		//erase requires toggle of reset line 
		//and so it is required to put it in program mode
		
		mux_programmode();
		/* NOTE THIS IS USED FOR BYTE PROGRAMMING MODE
		uint16_t i,addr=0;

		for(i=0;i<sizeof(AVR_binaries);i++)
		{
			addr=i>>1;
			uint8_t command=0x60|((i&&0x01)<<3);//will give 68 if highbyte or 60 if lowbyte
			writeresponse(command,(addr>>8)&0xFF,addr&0xFF,AVR_binaries[i]); 
		}
		*/
		// This is used as ATmega328p uses page programming mode
		uint16_t i;
		uint8_t command,addrLSB,addrMSB,LSB=0;

		for(i=0;i<sizeof(AVR_binaries);i++)
		{
			LSB=(i>>1)&0x3F;
			addrLSB=(i>>1)&0xFF;
			addrMSB=i>>9;
			command=0x40|((i&0x01)<<3);//will give 48 if highbyte or 40 if lowbyte
			
			mux_writecmd(command,0x00,LSB,AVR_binaries[i]);
			if( ( (i&0x7F)==0x7F ) || ( i==(sizeof(AVR_binaries)-1) ) )//127-255-etc.
			{//write the page
				mux_writecmd(0x4C,addrMSB,addrLSB,0x00);//Write Memory page command
			}
		}
	}
	else
	{//AVR did not respond
		TIMEOUT++;
		if(TIMEOUT<3)goto progRST;
		//try couple of times, if response[2] still isnt 0x53 there is a problem
	}
	RST(0);// PUT RESET HIGH (AVR has a !RESET)

}