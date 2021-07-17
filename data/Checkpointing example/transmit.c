#include "msp430.h"

#include "translib.h"

void main(void)
{  
  SystemInit();                             // Init the Board
  Task_Init();
  asm(" mov Task_head, SP");
//  UART_IO();
  Idle();
}


/**********************************************************************//**
 * @brief  Initializes system 
 * 
 * @param  none 
 *  
 * @return none
 *************************************************************************/
void SystemInit(void)
{  
  WDTCTL = WDTPW | WDTHOLD;                 // stop watchdog

  // Configure GPIO
  P1OUT &= ~BIT0;
  P1OUT |= BIT0;                           // Clear P1.0 output latch
  P1DIR |= BIT0 ;                            // For LED on P1.0

  P4OUT |= BIT6;
  P4OUT &= ~BIT6;
  P4DIR |= BIT6;
//  P4REN |= BIT5;
  P4IES &= ~BIT5;


  P4IE |= BIT5; // P4.5 interrupt enabled
  P4IFG =0; // P4.5 interrupt flag cleared


  P2SEL1 |= BIT0 | BIT1;                    // Configure UART pins
  P2SEL0 &= ~(BIT0 | BIT1);
  PJSEL0 |= BIT4 | BIT5;                    // Configure XT1 pins

  while(REFCTL0 & REFGENBUSY);
   REFCTL0 |= REFVSEL_1 + REFON;             //select internal reference 2.0 v
   __delay_cycles(400);
  // Configure ADC12
  ADC12CTL0 &= ~ADC12ENC;
  ADC12CTL0 = ADC12SHT0_2 | ADC12ON;        // Sampling time, S&H=16, ADC12 on
  ADC12CTL1 = ADC12SHP;                     // Use sampling timer
  ADC12CTL2 |= ADC12RES_2;                  // 12-bit conversion results
  ADC12CTL3 |= ADC12BATMAP;  				// battery voltage vcc/2
  ADC12MCTL0 |= ADC12INCH_3 | ADC12VRSEL_1;                // A3
 // ADC12MCTL0 |= ADC12INCH_31 | ADC12VRSEL_1;                // A31=Vcc/2
  ADC12IER0 |= ADC12IE0;                    // Enable ADC conv complete interrupt
  while(!(REFCTL0 & REFGENRDY));
  ADC12CTL0 |= ADC12ENC;



  // Disable the GPIO power-on default high-impedance mode to activate
  // previously configured port settings
  PM5CTL0 &= ~LOCKLPM5;

  // XT1 Setup
/*
  CSCTL0_H = CSKEY >> 8;                    // Unlock clock registers
  CSCTL1 = DCOFSEL_3 | DCORSEL;             // Set DCO to 8MHz
  CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
  CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
  CSCTL0_H = 0;                             // Lock CS registers
////////////////////////
  CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
  CSCTL1 = DCOFSEL_0;                       // Set DCO to 1MHz
  CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
  CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
  CSCTL4 &= ~LFXTOFF;
 */                      // Enable LFXT1

  CSCTL0_H = 0xA5;                          // Unlock register
  CSCTL1 |= DCOFSEL0 + DCOFSEL1;            // Set max. DCO setting
  CSCTL2 = SELA_1 + SELS_3 + SELM_3;        // set ACLK = vlo; MCLK = DCO
  CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;        // set all dividers
  do
  {
    CSCTL5 &= ~LFXTOFFG;                    // Clear XT1 fault flag
    SFRIFG1 &= ~OFIFG;
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
  CSCTL0_H = 0;                             // Lock CS registers

  
   TimerA0_Init();
}


void Task_Init()
{   Button45 = (unsigned int *) Addr_Button;
	CP_ptr = (unsigned int *) CP_Identifier;
	FRAM_PC_ptr = (unsigned int *) FRAM_CP_START;
	SP_temp = (unsigned int *) Stack_temp;
	*(CP_ptr+1)=1;//1=boot p; 0 = already in loop;
	/*****************initialize UART_IO*****************/
	Task[1].st_data[St_Size-1]=(unsigned int)UART_IO;
	Task[1].st_Ptr=(unsigned int*)&Task[1].st_data[St_Size-14];
	/****************initialize Idle*************************/
	Task[0].st_data[St_Size-1]=(unsigned int)Idle;
	Task[0].st_Ptr=(unsigned int*)&Task[0].st_data[St_Size-14];

	Task_head = (unsigned int*)&Task[0].st_data[St_Size-1];
	T_cur=0;
}


void UART_IO(void)//
{
  _EINT();
//  T_cur=0;
//  unsigned char c='a';
//  char Mimi[11]={0};
 //  Mimi[10]='\n';
   int S=sizeof(UART_Mimi);
   while(1)
   {
//	for(i=0;i<10;i++)
//	{
//		Mimi[i]=c++;
//	}
	_DINT();
	UARTx_PC(UART_Mimi, S);
	_EINT();
    P1OUT ^= BIT0;
	__delay_cycles(400000);
   }
}

void Idle(void)
{
	T_cur=0;


	_EINT();
	while(1)
	{
		__delay_cycles(100);
	}
}

/**********************************************************************//**
 * @brief  Long Delay
 * 
 * @param  none 
 *  
 * @return none
 *************************************************************************/
void LongDelay()
{
  __delay_cycles(250000);
  __no_operation();
}

/**********************************************************************//**
 * @brief  Transmit 7 bytes
 * 
 * @param  none 
 *  
 * @return none
 *************************************************************************/


void UARTx_PC(char string[], int size)
{
//  TimerA0_Disable();//distable timerA;
  unsigned int counter = 0;
  
// Configure USCI_A0 for UART mode

 /* UCA0CTL1 |= UCSWRST;
  UCA0CTL1 = UCSSEL__ACLK;                  // Set ACLK = 32768 as UCBRCLK
  UCA0BR0 = 3;                              // 9600 baud
  UCA0MCTLW |= 0x5300;                      // 32768/9600 - INT(32768/9600)=0.41                                           // UCBRSx value = 0x53 (See UG)
  UCA0BR1 = 0;
  UCA0CTL1 &= ~UCSWRST;                     // release from reset
*/
  UCA0CTL1 |= UCSWRST;
  UCA0CTL1 = UCSSEL_2;                      // Set SMCLK as UCLk
  UCA0BR0 = 52 ;                            // 9600 baud
  // 8000000/(9600*16) - INT(8000000/(9600*16))=0.083
  UCA0BR1 = 0;
  // UCBRFx = 1, UCBRSx = 0x49, UCOS16 = 1 (Refer User Guide)
  UCA0MCTLW = 0x4911 ;

  UCA0CTL1 &= ~UCSWRST;
/*
  UCA0CTLW0 = UCSWRST;                      // Put eUSCI in reset
  UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
  UCA0BR0 = 52;                             // 8000000/16/9600
  UCA0BR1 = 0x00;
  UCA0MCTLW |= UCOS16 | UCBRF_1;
  UCA0CTLW0 &= ~UCSWRST;
*/
   while(counter<size)
  {
    while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
    UCA0TXBUF = string[counter];
    __delay_cycles(20000);
    counter++;
  }
}

void TimerA0_Init()
{
  TA0CCTL0 = CCIE;                // TACCR0 interrupt enabled
  TA0CCR0 = 1000;
  TA0CTL = TASSEL_1 + MC_1;       // ACLK, up mode
//  __bis_SR_register(LPM4_bits + GIE);
}

/******************checkpointing*************/   
void Checkpointing()
{	int i;
	CP_Data=(unsigned int*)Task;
	for(i=0; i<sizeof(Task); i++)
	{
		if(*(CP_Data+i)!=*(FRAM_PC_ptr+i))
		{
			*(FRAM_PC_ptr+i)=*(CP_Data+i);
		}
	}
	if(UCA0TXBUF!=*(FRAM_PC_ptr+sizeof(Task)))
	{
	  *(FRAM_PC_ptr+sizeof(Task))=UCA0TXBUF;
	}
	*CP_ptr = 1;// 1=has been checkpointed
}

/*********resumming**************/
void Resumming()
{	int i;
    CP_Data=(unsigned int*)Task;
	for(i=0; i<sizeof(Task); i++)
	{
		if(*(CP_Data+i)!=*(FRAM_PC_ptr+i))
		{
			*(CP_Data+i)=*(FRAM_PC_ptr+i);
		}
	}
    if(UCA0TXBUF!=*(FRAM_PC_ptr+sizeof(Task)))
    {
//    	UCA0TXBUF=*(FRAM_PC_ptr+sizeof(Task));  // if the UART data resumming is not right uncomment this instruction
    }
	*(CP_ptr+1)=0;
}

void VMonitor()
{
 	if(*CP_ptr==1 && *(CP_ptr+1)==1 && *Button45==0){//if it has been checkpointted (1) and if it has been resummed (0)
 		T_cur=0;//switch to Idle;
 	    SP_ptr = Task[T_cur].st_Ptr;
 	    P4OUT |= BIT6;
	}
 	else if(*CP_ptr==1 && *(CP_ptr+1)==1 && *Button45==1){//if it has been checkpointted (1) and if it has been resummed (0)
 		Resumming();
 		*Button45=0;
 		Task[T_cur].st_Ptr = SP_ptr;
 			ADC12CTL0 |= ADC12SC;
 			while(!(ADC12IFGR0 & BIT0));
 			Voltage = ADC12MEM0;
 			if(*CP_ptr==1)//has been checkpointed
 		//		P1OUT ^= BIT0; //Flash
 				P4OUT |= BIT6; //Always on
 			if(T_cur==0)// current in Idle
 				P4OUT |= BIT6;
 		 	if(*(CP_ptr+1)==0)// has been resumed
 				P4OUT |= BIT6;

 			if(Voltage < 2800 && T_cur==1)    //Vcc < 3v
 			{   P4OUT ^= BIT6;
 				Checkpointing();
 				T_cur=0;//switch to Idle;
 				SP_ptr = Task[T_cur].st_Ptr;
 			}
 			if(Voltage >= 2800)
 			{
 				T_cur=1;//switch to UARTx_IO;
 				SP_ptr = Task[T_cur].st_Ptr;
 			}
 		    __delay_cycles(200);
 	}
 	else{
	Task[T_cur].st_Ptr = SP_ptr;
	ADC12CTL0 |= ADC12SC;
	while(!(ADC12IFGR0 & BIT0));
	Voltage = ADC12MEM0;
	if(*CP_ptr==1)//has been checkpointed
//		P1OUT ^= BIT0; //Flash
		P4OUT |= BIT6; //Always on
	if(T_cur==0)// current in Idle
		P4OUT |= BIT6;
 	if(*(CP_ptr+1)==0)// has been resumed
		P4OUT |= BIT6;

	if(Voltage < 2800 && T_cur==1)    //Vcc < 3v
	{   P4OUT ^= BIT6;
		Checkpointing();
		T_cur=0;//switch to Idle;
		SP_ptr = Task[T_cur].st_Ptr;
	}
	if(Voltage >= 2800)
	{
		T_cur=1;//switch to UARTx_IO;
		SP_ptr = Task[T_cur].st_Ptr;
	}
    __delay_cycles(200);
 	}
/***************round-robin for testing***************/
	/*
	if(T_cur==0)    //Vcc < 3v
	{
//		Checkpointing();
		T_cur=1;//switch to Idle;
		SP_ptr = Task[T_cur].st_Ptr;
	}
	else if(T_cur==1)
	{
		T_cur=0;//switch to UARTx_IO;
		SP_ptr = Task[T_cur].st_Ptr;
	}
	*/

}



#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
	*Button45=1;  //  Once Button p4.5 is pressed, value of Button45 is set to 1;
    P4IFG &= ~BIT5; // P4.5 interrupt flag cleared
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
  asm(" push.w R4");
  asm(" push.w R5");
  asm(" push.w R6");
  asm(" push.w R7");
  asm(" push.w R8");
  asm(" push.w R9");
  asm(" push.w R10");
  asm(" push.w R11");
  asm(" push.w R12");
  asm(" push.w R13");
  asm(" push.w R14");
  asm(" push.w R15");
  asm(" mov SP, SP_ptr");
  asm(" calla #VMonitor");
  asm(" mov SP_ptr, SP");
  asm(" pop.w R15");
  asm(" pop.w R14");
  asm(" pop.w R13");
  asm(" pop.w R12");
  asm(" pop.w R11");
  asm(" pop.w R10");
  asm(" pop.w R9");
  asm(" pop.w R8");
  asm(" pop.w R7");
  asm(" pop.w R6");
  asm(" pop.w R5");
  asm(" pop.w R4");
}

