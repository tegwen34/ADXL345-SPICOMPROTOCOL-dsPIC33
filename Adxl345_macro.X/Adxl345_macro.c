#include "pic24_all.h"
#include <stdio.h>

#define DEVID           0x00
#define BW_RATE         0x2C
#define POWER_CTL       0x2D
#define DATA_FORMAT     0x31
#define DATAX0          0x32
#define DATAX1          0x33
#define DATAY0          0x34
#define DATAY1          0x35
#define DATAZ0          0x36
#define DATAZ1          0x37 
#define FIFO_CTL        0x38

#define CONFIG_SLAVE_ENABLE() CONFIG_RB5_AS_DIG_OUTPUT()
#define SLAVE_ENABLE()        _LATB5 = 0  //high true assertion
#define SLAVE_DISABLE()       _LATB5 = 1

unsigned char device_id;


void configSPI1(void) {
  //spi clock = 70MHz/(5*4) = 70MHz/20 = 3.5MHz
  SPI1CON1 = SEC_PRESCAL_5_1 |     //1:1 secondary prescale
             PRI_PRESCAL_4_1 |     //4:1 primary prescale
             CLK_POL_ACTIVE_LOW   | //idle state for clock is a high level, active is low level  (CKP = 1)
             SPI_CKE_OFF          | //out changes inactive to active (CKE=0)
             SPI_MODE8_ON        | //8-bit mode
             MASTER_ENABLE_ON;     //master mode
#if (defined(__dsPIC33E__) || defined(__PIC24E__))
  //nothing to do here. On this family, the SPI1 port uses dedicated
  //pins for higher speed. The SPI2 port can be used with remappable pins.
#else
//all other families (PIC24H/PIC24F/dsPIC33F)
  CONFIG_SDO1_TO_RP(6);      //use RP6 for SDO
  CONFIG_RP6_AS_DIG_PIN();   //Ensure that analog is disabled
  CONFIG_SCK1OUT_TO_RP(7);   //use RP7 for SCLK
  CONFIG_RP7_AS_DIG_PIN();   //Ensure that analog is disabled
  CONFIG_SDI1_TO_RP(5);      //use RP5 for SDI
  CONFIG_RP5_AS_DIG_PIN();   //Ensure that analog is disabled
#endif
  CONFIG_SLAVE_ENABLE();       //chip select for MCP41xxx
  SLAVE_DISABLE();             //disable the chip select
  SPI1STATbits.SPIEN = 1;  //enable SPI mode
}

void writeConfigADXL345(uint8_t address,uint8_t value) {
  SLAVE_ENABLE();          //assert chipselect
  ioMasterSPI1(address);   //config address
  ioMasterSPI1(value);     //config value
  SLAVE_DISABLE();
}

int16_t readADXL345(uint8_t address) {
  //uint16_t u16_lo, u16_hi;
  uint8_t temp; 
  address = 0x80 | address;// reading mode
  SLAVE_ENABLE();       //assert chipselect
  ioMasterSPI1(address);   //address
  temp = ioMasterSPI1(0x00); //read LSByte
  //u16_hi = ioMasterSPI1(0x00); //read MSByte
  SLAVE_DISABLE();
  return(temp);
}

int main (void) {
  int16_t i16_temp;
  float  f_tempC,f_tempF;
  int xhi,xlo,yhi,ylo,zhi,zlo;
  long Xaccumulate, Yaccumulate, Zaccumulate;
  uint8_t i;
  configBasic(HELLO_MSG);
  configSPI1();
  
  //DATA_FORMAT = 0x09 
  writeConfigADXL345(DATA_FORMAT,0x09); //config address,config value, Full_res (11 bits),-+4g, 3.90625 mg
  //POWER_CTL = 0x08 
  writeConfigADXL345(POWER_CTL,0x08 ); //config address,config value, measurement mode
   //BW_RATE = 0x0d 
  writeConfigADXL345(BW_RATE,0x0d); //config address, config value, measurement mode
 
   device_id=readADXL345(DEVID);
   printf("DEVID = %d\r\n",device_id);
  
  

  while (1) 
  {
    Xaccumulate = Yaccumulate = Zaccumulate = 0;
    for (i=0; i<16; i++)
        { // Read sequentially 16 times then get an average.
            xlo=readADXL345 (DATAX0);    // DATAX0 is the first of 6 bytes 
            xhi=readADXL345 (DATAX1);    // read character
            ylo=readADXL345 (DATAY0);    // read character 
            yhi=readADXL345 (DATAY1);    // read character
            zlo=readADXL345 (DATAZ0);    // read character
            zhi=readADXL345 (DATAZ1);    // read character
             
            Xaccumulate += ((xhi<<8) | xlo); //Xaccumulate = Xaccumulate + (xhi*256 + xlo) 
            Yaccumulate += ((yhi<<8) | ylo);
            Zaccumulate += ((zhi<<8) | zlo);
        }
    Xaccumulate=Xaccumulate>>4;
    Yaccumulate=Yaccumulate>>4;
    Zaccumulate=Zaccumulate>>4;
    printf("X = %ld (%.2f g), Y=%ld (%.2f g), Z=%ld (%.2f g) \r\n",Xaccumulate,Xaccumulate * 0.00390625,Yaccumulate,Yaccumulate * 0.00390625,Zaccumulate,Zaccumulate * 0.00390625);
      
      DELAY_MS(1000);
     
   /*
    DELAY_MS(1500);
    i16_temp = readADXL345();
    f_tempC = i16_temp;  //convert to floating point
    f_tempC = f_tempC/256;  //divide by precision
    f_tempF = f_tempC*9/5 + 32;
    printf("Temp is: 0x%0X, %4.4f (C), %4.4f (F)\n", i16_temp, (double) f_tempC, (double) f_tempF);
    
    */
    
  }
}