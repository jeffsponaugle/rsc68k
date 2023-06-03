/*

  Available PA7-0; PB7-0; PC7-0; PD7,3-0; PE5,4,3,1,0; PF7-0; PG5,2,1,0; PH6,5,4,3,1,0; PJ1,0; PK7-0, PL7-0;
  Device Hookup:
  PG = PG0=CLK_32(out) Pin[18], PG1=CLK_16(in) Pin[30] ,PG2=CPU_RESET(in) Pin[29], PG5=RESET_IN(out) Pin[17] 
 
   PC = DATABUS 8-15 Not used this test.
  PF = ADDRESSBUS 7-0 PF3-1 A3-A1 = Pin[56,57,58]
  PL = INTERRUPTS (PL7-0) IRQ6A,6B,5A,5B,4A,4B,3A,3B = Pin[84,1,2,4,5,6,8,9]
   PK = ADDRESSBUS 23-16 (PK7-0) = [Pin 17,18,20,21,22,24,25,27]
  PE = PE5,4,3=FC2-0 (PE5,4,3) = [Pin 24,22,21]
   PD = PD0=/AS Pin[6], PD1=R-/W Pin[2], PD2=/UDS Pin[5], PD3=/LDS Pin[4], PD7=DTACK_FROM_INTT Pin[41]
  
  INPUTS TO VERIFY
  PA = DATABUS0-7 - D7-D0(PA7-0) on [Pins 48,45,44,41,40,39,37,36]
   PJ = PJ0=DTACK Pin[45],  PJ1=DRAM_CS Pin[70]
   PH = PH0=SRAM0_LOW_CS Pin[79],  PH1=SRAM0_HIGH_CS Pin[77],  PH3=MEM_OE Pin[15],  PH4=RTS_CS Pin[57],  PH5=RTS_A_SEL Pin[56],  PH6=PTC_CS Pin[55]
   PB = PB0=FLASH_LOW_CS Pin[76],  PB1=FLASH_HIGH_CS Pin[75],  PB2=FLASH_A18 Pin[74],  PB3=LED_LATCH Pin[80]  PB4=IDE_CSA Pin[65],  PB5=UART_CSA Pin[63],  PB6=SEVENSEG_LATCH_CS Pin[60],  PB7=LED
   
 */ 


#define MEMREAD 1
#define MEMWRITE 0

int g_bus_state = 0;
char buffer[256];

void setup() 
{
  Serial.begin(115200);
  DatabusASTristate();
  AddressbusASOutput();
  /* PG0=CLK_32(out), PG1=CLK_16(out),PG2=RESET(out), PG5=not used */
  DDRG = 0b00000111; /* CLK32,CLK16, RESET outputs */
  PORTG = 0b00000000;
  
  DDRE = 0b00111000; /* FC outputs */
  PORTB = 0b00000000;
  
  DDRD = 0b10001111; /* AS RW UDS LDS , DTACK_FROM_INT outputs */
  PORTD = 0b10001111;
   
  DDRB =  0b10000000; /* LED output */
  PORTB = 0b00000000;
  
  SetResetIn();
  SetAddr(0);
  SetFC(0);
  SetAS(1);
  SetRW(1);
  SetUDS(1);
  SetLDS(1);
 
  /* device in reset */
}


void loop() {


  /* Start by releasing the reset_in signal.*/
  Serial.print("Testing reset vector\r\n");
  ShowState();
  MoveClockToZero();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  
  Serial.print("Releaseing RESET\r\n");
  UnsetResetIn();
  ShowState();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  Serial.print("Toggle CLK_16\r\n");
  HalfClock();
  ShowState();
  Serial.print("Moving to both clocks zero.\r\n");
  MoveClockToZero();
  ShowState();

  while (1==1);
  /* Lets run the clock for 4 full cycles with no bus activity */
   Serial.print("Running for 4 clocks cycles, no bus activity\r\n");

  for (int i=0;i<16;i++)
  {
    QtrClock();
    ShowState();
  }
  Serial.print("\r\nWe should be back at CLK_16 zero, ready to start testing address decode.\r\n");
  ShowState();
  Serial.print("Let's fetch the first vector word and stack ptr. \r\n");


  /* void BusCycle(int readwrite,long addr,int low, int high, int data) */

  BusCycle(MEMREAD,0,1,1,0);  /* Read addr 0 */
  BusCycle(MEMREAD,2,1,1,0);  /* Read addr 2 */
  BusCycle(MEMREAD,4,1,1,0);  /* Read addr 4 */
  BusCycle(MEMREAD,6,1,1,0);  /* Read addr 6 */

  Serial.print("Now lets jump into the ROM area, fetching an instruction from that area.. \r\n");
  BusCycle(MEMREAD,0xF00000,1,1,0);
  BusCycle(MEMREAD,0xF00002,1,1,0);

  Serial.print("We will now test read/write/read from low memory.   Read AA, Write 1122 to BB, Read CC \r\n");
  BusCycle(MEMREAD,0x0000AA,1,1,0);
  BusCycle(MEMWRITE,0x0000BB,1,1,0x1122);
  BusCycle(MEMREAD,0x0000CC,1,1,0);
  
  Serial.print("We will now do a read/write/read from the DRAM Area.  Read 100AA, Write 3344 to 100BB, Read 100CC \r\n");
  
  BusCycle(MEMREAD,0x0100AA,1,1,0);
  BusCycle(MEMWRITE,0x0100BB,1,1,0x3344);
  BusCycle(MEMREAD,0x0100CC,1,1,0); 

  Serial.print("We will now do a read from the FLASH Area, which should map to FLASH (A18=0) \r\n");
  Serial.print("     read F100AA hl, read F100AA high, read F100AA low \r\n");
  BusCycle(MEMREAD,0xF100AA,1,1,0);
  BusCycle(MEMREAD,0xF100AA,1,0,0);
  BusCycle(MEMREAD,0xF100AA,0,1,0);

  Serial.print("We will now do a write to the FLASH Area, which should map to SRAM \r\n");
   Serial.print("     write F100AA hl, write F100AA high, write F100AA low \r\n");
  BusCycle(MEMWRITE,0xF100AA,1,1,0x5566);
  BusCycle(MEMWRITE,0xF100AA,1,0,0x5566);
  BusCycle(MEMWRITE,0xF100AA,0,1,0x5566);

  Serial.print("We will now do a write to the FLASH_CONFIG=0b1001(0x9) Register to change to WRITE=FLASH,A18=0, READ=FLASH,A18=1 \r\n");
  BusCycle(MEMWRITE,0xFF8700,1,0,0x9);

  Serial.print("We will now do a read from the FLASH Area, which should map to FLASH (A18=1) \r\n");
  BusCycle(MEMREAD,0xF100AA,1,1,0);

  Serial.print("We will now do a write to the FLASH Area, which should map to FLASH (A18=0) \r\n");
  BusCycle(MEMWRITE,0xF100AA,1,1,0x7788);

  Serial.print("We will now do a write to the FLASH_CONFIG=0b1110(0xE) Register to change to WRITE=FLASH,A18=1, READ=SRAM \r\n");
  BusCycle(MEMWRITE,0xFF8700,1,0,0xE);

  Serial.print("We will now do a read from the FLASH Area, which should map to SRAM \r\n");
  BusCycle(MEMREAD,0xF100AA,1,1,0);

  Serial.print("We will now do a write to the FLASH Area, which should map to FLASH(A18=1) \r\n");
  BusCycle(MEMWRITE,0xF100AA,1,1,0x7788);

  Serial.print("We will now do a write to the FLASH_CONFIG=0b0000(0x0) Register to change to WRITE=SRAM, READ=FLASH,A18=0 \r\n");
  BusCycle(MEMWRITE,0xFF8700,1,0,0x0);

  Serial.print("We will now do a read from the FLASH Area, which should map to FLASH (A18=0) \r\n"); 
  BusCycle(MEMREAD,0xF100AA,1,1,0);

  Serial.print("We will now do a write to the FLASH Area, which should map to SRAM \r\n");
  BusCycle(MEMWRITE,0xF100AA,1,1,0x99AA);

  Serial.print("We will now test read from low memory lower 8 bits. SRAM \r\n");
  BusCycle(MEMREAD,0x0000AB,1,0,0);

  Serial.print("We will now test read from low memory upper 8 bits  SRAM \r\n");
  BusCycle(MEMREAD,0x0000AC,0,1,0);

  Serial.print("We will now test read and write from/to IDE \r\n");
  BusCycle(MEMREAD,0xFF0300,0,1,0);
  BusCycle(MEMWRITE,0xFF0300,0,1,0xBBCC);

  Serial.print("We will now test read and write from/to UARTA \r\n");
  BusCycle(MEMREAD,0xFF0100,0,1,0);
  BusCycle(MEMWRITE,0xFF0100,0,1,0xDDEE);

  Serial.print("We will now test write from/to 7Segment \r\n");
  BusCycle(MEMWRITE,0xFF0000,0,1,0xDDEE);

  Serial.print("We will now test read and write from/to RTC \r\n");
  BusCycle(MEMREAD,0xFF8500,0,1,0);
  BusCycle(MEMWRITE,0xFF8500,0,1,0xFF00);

  Serial.print("We will now test read and write from/to PTC \r\n");
  BusCycle(MEMREAD,0xFF8600,0,1,0);
  BusCycle(MEMWRITE,0xFF8600,0,1,0xF101);

  Serial.print("We will now test write to 7SEG (16 bit) \r\n");
  BusCycle(MEMWRITE,0xFF0000,1,1,0xF102);

  Serial.print("We will now test write to LED \r\n");
  BusCycle(MEMWRITE,0xFF0500,0,1,0xF103);
   
  
  Serial.print("TEST END\r\n");
    Serial.print("Running for 4 clocks cycles, no bus activity\r\n");

  for (int i=0;i<16;i++)
  {
    QtrClock();
    ShowState();
  }
  while (1==1);   
}

void BusCycle(int readwrite,long addr,int low, int high, int data)
{
  
  Serial.print("####################################################################################################################\r\n");
  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=0;
  /* Stuff that happens entering state 0 */
  ShowState();
 
  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=1;
  /* Stuff that happens entering state 1 */
  SetAddr(addr);
  ShowState();

  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=2;
  /* Stuff that happens entering state 2 */
  SetAS(0);   /* Assert AS */
  if (high!=0) SetUDS(0); 
  else SetUDS(1); 
  if (low!=0) SetLDS(0);
  else SetLDS(1);
  if (readwrite == 0)
  {
    SetRW(0);
    DatabusASOutput();
    SetData(data);
  }
  else
  {
    SetRW(1);
  }
  ShowState();

  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=3;
  /* Stuff that happens entering state 3 */
  
  ShowState();

  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=4;
  /* Stuff that happens entering state 4 */
  
  ShowState();

  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=5;
  /* Stuff that happens entering state 5 */
  
  ShowState();
  
  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=6;
  /* Stuff that happens entering state 6 */
  
  ShowState();
  
  QtrClock();
  ShowState();
  QtrClock();
  g_bus_state=7;
  /* Stuff that happens entering state 7 */
  SetAS(1);
  SetUDS(1);
  SetLDS(1);
  SetRW(1);
  DatabusASTristate();
  ShowState();
  
}

/* Support Functions for setup */


void DatabusASTristate()
{
  DDRA = 0;
  DDRC = 0;
  PORTA = 0;
  PORTC = 0;

}

void DatabusASOutput()
{
  PORTA = 0x00;
  PORTC = 0x00;
  DDRA = 0xFF;
  DDRC = 0xFF;
  }

void AddressbusASOutput()
{
  PORTK = 0x00;
  PORTL = 0x00;
  PORTF = 0x00;
  DDRK = 0xFF;
  DDRL = 0xFF;
  DDRF = 0xFF;
}

/* Functios to set output states */

void SetData(int val)
{
  PORTA = (val & 0xFF);
  PORTC = (val >> 8) & 0xFF;
}

void SetAddr(long val)
{
  PORTF = val & 0xFF;
  PORTL = (val>>8) & 0xFF;
  PORTK = (val>>16) &0xFF; 
}

void SetFC(int val)
{
  PORTE = (val&0b00000111)<<3;
}

void SetAS(int val)
{
  if (val==0)
    PORTD &= 0b11111110;
  else
    PORTD |= 0b00000001;
}

void SetRW(int val)
{
  if (val==0)
    PORTD &= 0b11111101;
  else
    PORTD |= 0b00000010;
}

void SetUDS(int val)
{
  if (val==0)
    PORTD &= 0b11111011;
  else
    PORTD |= 0b00000100;
}

void SetLDS(int val)
{
  if (val==0)
    PORTD &= 0b11110111;
  else
    PORTD |= 0b00001000;
}

void SetResetIn()
{
   PORTG &= 0b11111011;   /* device in reset */
}

void UnsetResetIn()
{
  PORTG |= 0b00000100;
}

/* Clock Functions */

void QtrClock()
{
  
  /* Advance the machine 1/4 clock cycle, which is 1 toggle of CLK_32.
   *  
   *  CLK_16 toggles if CLK_32 goes high.
   */
   PORTG ^= 0b00000001;
   if ((PING & 0b00000001) == 1) PORTG ^=0b00000010;
   
   PORTB ^= 0b10000000; /* Toggle B7 (LED) */
}

void HalfClock()
{ 
  QtrClock();
  QtrClock();
}

void FullClock( int cycles)
{
  for (int i=0; i<cycles; i++)
  {
   HalfClock();
   HalfClock();
  }
}
void MoveClockToZero()
{
  /* advance the clocks so both clocks are at 0 */
  if ( (PING&0b00000010)==0 )
      /* CLK_16=0  */
   {  
      if ( (PING&0b00000001)==0 )
      {
        /* CLK_32=0, CLK_16=0 */
        /* We have nothing to do here */
      }
      else
      {
        /* CLK_32=1, CLK_16=0  A single clk_32 toggle will get both clocks low */
        QtrClock();
      }
   }
   else
   {
      if ( (PING&0b00000001)==0 )
      {
        /* CLK_32=0, CLK_16=1.  Toggle CLK_32 twice to get both clocks low */
         QtrClock();
         QtrClock();
      }
      else
      {
        /* CLK_32=1, CLK_16=1. Toggle CLK_32 three times to get both clocks low   */
        QtrClock();
        QtrClock();
        QtrClock();
      }
   }
   /* at this point CLK_16 and CLK_32 is zero, so next clock cycle will be a rising clock on both */
}

void Clock32Low()
{
  PORTG &= 0b11111110; /* set clock32 low */
}

/* Print States */

void ShowState()
{
  
  int CLK32=(PING & 1);
  int CLK16=(PING & 0b00000010)>>1;
  int CPURESET=(PING & 0b00000100)>>2;
  int RESETIN=(PING & 0b00100000 )>>5;
  int FC=(PINE &0b00111000) >> 3;
  int DATA=(PINA|(PINC<<8));
  int DTACK =  (PINJ & 0b00000001);
  int MOE =    (PINH & 0b00001000) >> 3;
  int DRAMCS = (PINJ & 0b00000010) >> 1;
  int SRAML =  (PINH & 0b00000001);
  int SRAMH =  (PINH & 0b00000010) >> 1;
  int FLSHL =  (PINB & 0b00000001);
  int FLSHH =  (PINB & 0b00000010) >> 1;
  int FLSH18 = (PINB & 0b00000100) >> 2;
  int LEDCS  = (PINB & 0b00001000) >> 3;
  int IDE    = (PINB & 0b00010000) >> 4;
  int UART   = (PINB & 0b00100000) >> 5;
  int SSG    = (PINB & 0b01000000) >> 6;
  int RTS    = (PINH & 0b00010000) >> 4;
  int RTSAD  = (PINH & 0b00100000) >> 5;
  int PTC    = (PINH & 0b01000000) >> 6;
  int AS     = (PIND & 0b00000001);
  int RW     = (PIND & 0b00000010) >> 1;
  int UDS    = (PIND & 0b00000100) >> 2;
  int LDS    = (PIND & 0b00001000) >> 3;
  
  long ADDR=(PINF | (PINL<<8) | ((long)PINK<<16));
  sprintf(buffer,"%1u/%1u[S%u] \tRR :%1u%1u FC:%1u DATA:%04x ADDR:%06lx ",CLK16,CLK32,g_bus_state,RESETIN,CPURESET,FC,DATA,ADDR);
  Serial.print(buffer);
  sprintf(buffer,"AS:%1u RW:%1u UDS:%1u LDS:%1u\r\n",AS,RW,UDS,LDS);
  Serial.print(buffer);
  sprintf(buffer,"\t\tDTACK:%1u MOE:%1u DRAM:%1u SRAML:%1u SRAMH:%1u FLSHL:%1u FLSHH:%1u FLSH18:%1u LEDCS:%1u IDE:%1u UART:%1u 7SG:%1u RTS:%1u RTSAD: %1u PTC:%1u\r\n", 
          DTACK,MOE,DRAMCS,SRAML,SRAMH,FLSHL,FLSHH, FLSH18, LEDCS, IDE, UART, SSG, RTS, RTSAD, PTC );
  Serial.print(buffer);
  
}
