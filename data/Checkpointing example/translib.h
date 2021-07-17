/*******************************************************************************
 *
 * main.c
 * User Experience Code for the MSP-EXP430FR5739
 * 
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Created: Version 1.0 04/13/2011
 *          Version 1.1 05/11/2011
 *          Version 1.2 08/31/2011
 ******************************************************************************/
#include "msp430fr5969.h"

// Mode Definitions
   
#define ROUND_ROBIN 0
#define NVP_Scheduler 1
#define Checkpoint 2
#define Sleep  3
#define Capacitance 470
#define drop 2
#define rise 8
   

// The FRAM section from 0xD000 - 0xF000 is used by all modes
// for performing writes to FRAM
// Do not use this section for code or data placement. 
// It will get overwritten!
//#define ADC_START_ADD 0xE400
//#define ADC_END_ADD 0xE300
//#define FRAM_TEST_START 0xD400
//#define FRAM_TEST_END 0xF000
//#define MEM_UNIT 0x200

//#define Stack_CP  0xE100
   
   
#define FRAM_CP_START 0xF000
#define CP_Identifier 0xFE00
#define Addr_Button 0xE000
//#define Count_addr 0xE102
#define Stack_temp 0xFF00
//#define STACK_HEAD 0x1FFC

////    FRAM2                   : origin = 0x10000,length = 0x4000
//    FRAM2                   : origin = 0x10000,length = 0x3000   //peter modified
//    Mimi                    : origin = 0x13000, length = 0x1000  //peter modified



#define T_Num               2
//#define T_Cycle             10000
#define T_ready             0
#define T_run               1
#define St_Size             43
#define Cush_Size           8
#define Stb_max             10
#define Stb_min             0

// Pin Definitions
#define ACC_PWR_PIN       BIT7
#define ACC_PWR_PORT_DIR  P2DIR
#define ACC_PWR_PORT_OUT  P2OUT
#define ACC_PORT_DIR      P3DIR
#define ACC_PORT_OUT      P3OUT
#define ACC_PORT_SEL0     P3SEL0
#define ACC_PORT_SEL1     P3SEL1
#define ACC_X_PIN         BIT0
#define ACC_Y_PIN         BIT1
#define ACC_Z_PIN         BIT2

#define cp_Status         BIT8

// Accelerometer Input Channel Definitions
#define ACC_X_CHANNEL     ADC10INCH_12
#define ACC_Y_CHANNEL     ADC10INCH_13
#define ACC_Z_CHANNEL     ADC10INCH_14

#define DOWN 0
#define UP 1

#define CK_size 100
/*
#if defined(__TI_COMPILER_VERSION__)
#pragma PERSISTENT(FRAM_write)
unsigned int CK_FRAM[CK_size] = {0};
#define FRAM_CP_START 0xF000
#define CP_Identifier 0xF100
#define Stack_temp 0xF200
//#elif defined(__IAR_SYSTEMS_ICC__)
//__persistent unsigned int CK_FRAM[CK_size] = {0};
#else
#error Compiler not supported!
#endif
*/

// Function Declarations

volatile unsigned char UserInput = 0;
volatile unsigned char ULP =0; 
volatile unsigned int *FRAMPtr = 0;
volatile unsigned int *Task_head;
volatile unsigned char active = 0;
//volatile unsigned char SwitchCounter=0;
//volatile unsigned char Switch1Pressed=0;
//volatile unsigned char Switch2Pressed=0;
volatile unsigned int ADCResult = 0;
volatile unsigned int ACC_X=0;
volatile unsigned int ACC_Y=0;
volatile unsigned int ACC_Z=0;


unsigned int ADCTemp=0;
unsigned int CalValue_ACC = 0;
unsigned int CalValue_Therm = 0;
unsigned int LEDCounter=0;
unsigned char ThreshRange[3]={0,0,0};
unsigned char counter =0;
unsigned int WriteCounter = 0;
unsigned char ULPBreakSync =0;
unsigned int robin_Delay=0;
unsigned char temp = 0;
unsigned int *FRAM_PC_ptr;//fram address for checkpointing
unsigned int *CP_Data;//data need to be checkpointed
unsigned int *CP_ptr;//checkpointing state indicator
unsigned int *SP_cur;//used for checkpointing divert address
unsigned int *Button45;//used for saving state of button p4.5
unsigned int *SP_ptr;
unsigned int *SP_temp;
unsigned int *SP_cp;
unsigned int *Sys_head;
unsigned int *SP_port4;
unsigned int size;

unsigned int NA=0;// 1: All non-atomic tasks has been scheduled 
unsigned char port4_cp=0;
int T_cur;//current task ID
unsigned char Ct=0;
unsigned char schedule_enable=0;;
unsigned char mode;
unsigned int checkpoint;

//unsigned char UserInput = 0;
//unsigned char ULP =0; 
//unsigned char active = 0;
//unsigned char SwitchCounter=0;
//unsigned char Switch1Pressed=0;
//unsigned char Switch2Pressed=0;

// These golabal variables are used in the ISRs and in FR_EXP.c
//volatile unsigned int checkpoint; //modified by pc 06/02/2015
//unsigned long checkpoint;


//unsigned char TX_Buffer[7] = {0,0,0,0,0,0,0};
/*
//char TX_Welcome[]={"Welcome to Use NVOS on MSP430FR5739\n*******Presented by Peter and Mimi********\n"};
//char TX_Accer_Prog[]={"\nCurrent task: Acceleration Measurement \n"};
//char TX_Thermistor[]={"\nCurrent task: Temperature Sensing \n"};
//char TX_Comutation[]={"\nCurrent task: Computation \n"};
//char TX_Robin[]={"\nStart Round-Robin Scheduling\n"};
//char TX_Background[]={"\nStart Background Task and All Customer Tasks Are Hanging! \n"};
//char TX_Acceleration[]={};//modify at 2,3   7,8   12,13
//char TX_PAI[]={"Pi=  \r\n"};//modifiy at 3,4
//char ACC[]={"X=  \n"};//5(l,r), 6(+ -), 7(value)
//char Temperature[]={"Temperature Variation=  C \n"};//12, 13
//char Compute[]={"N=     \n"};//2,3,4,5,6,
*/
char variation;
unsigned char V_count=0;//the number of samples
unsigned char Tsk_temp=0;
float Pow=0;
float D_Pow=0;
float trust_P=0;
unsigned int Voltage=0;
unsigned int Power[4]={0};
unsigned int D_power[3]={0};
//char UART_Progress[]={"0x00, , , , , ,0x01, , , , , ,0x02, , , , , ,0x03, , , , , ,0x04, , , , , ,0x05, , , , , ,0xFF\n"};
char UART_Mimi[] ={"abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"};
//const unsigned char LED_Menu[] = {0x80,0xC0,0xE0,0xF0};


volatile struct
{
   unsigned int *st_Ptr;//stack pointer
   unsigned int st_data[St_Size];//stack size
   unsigned int st_cushion[Cush_Size];//used for cushion gap
}
Task[T_Num];//0:background  1:task A   2:task B   3:task C






//volatile unsigned int *FRAM_PC_ptr;
//unsigned long *FRAM_PC_ptr;



void SystemInit(void);
void StartUpSequence(void);
void FRAM_Write(unsigned int StartAddress);
void Idle(void);
void Cmp_A(void);
void Cmp_B(void);
void Cmp_C(void);
void Acceleration(void);
void Thermistor(void);
unsigned int CalibrateADC(void); 
void TakeADCMeas(void);
void SetupAccel(void); // setup ADC for Acc
void ShutDownAccel(void);
void SetupThermistor(void);
void ShutDownTherm(void);
void LEDSequence(unsigned int, unsigned char);
void LEDSequenceWrite(unsigned char);
//void DisableSwitches(void);
//void EnableSwitches(void);
void StartDebounceTimer(unsigned char);
void LongDelay(void);
void TXData(void);
//void TXBreak(unsigned char);
void UARTx_PC(char string[], int size);
void Task_Init();
void Scheduler(void);
void feed_Dog(void);
//void Port_4_Operation(void);
void TimerA0_Init();
void TimerB_Init();
void TimerA0_Disable();
void Sys_Start();
void Led_Turnoff();
void TimerA0_Disable();
void SetupVMeas(void);
void ShutDownVMeas(void);
void Checkpointing();
void VMonitor(void);
void task_Handler(void);//selection between voltage monitoring and task schedling
void UART_IO(void);
//void Computation2(void);//CALCULATING PI for testing purpose
