#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"

#define HXT_STATUS 1<<0
#define PLL_STATUS 1<<2
#define PLLCON_FB_DV_VAL 10
#define CPUCLOCKDIVIDE 1
#define TIMER0_COUNTS (SystemCoreClock / 1000000 - 1) // 1us
// System states
typedef enum {
    IDLE,
    ALARM_SET,
    COUNT,
    PAUSE,
    CHECK
} SystemState;

volatile SystemState currentState = IDLE;

// Global variables
volatile uint8_t alarmTime = 0;    // Alarm time (seconds)
volatile uint16_t elapsedTime = 0; // Elapsed time (1/10 seconds)
volatile uint16_t lapTimes[5] = {0}; // Lap times
volatile uint8_t lapIndex = 0;
volatile uint8_t checkLapIndex = 0;

//delay time for debouncing keys
#define BOUNCING_DLY 200000

// 7-segment patterns for digits
int pattern[] = {
    0b10000010,  // Number 0
    0b11101110,  // Number 1
    0b00000111,  // Number 2
    0b01000110,  // Number 3
    0b01101010,  // Number 4
    0b01010010,  // Number 5
    0b00010010,  // Number 6
    0b11100110,  // Number 7
    0b00000010,  // Number 8
    0b01000010,  // Number 9
    0b11111111   // Blank LED
};
uint8_t lap_num=0;
// Function prototypes
void System_Config(void);
void SetUp(void);
void KeyPadEnable(void);
uint8_t KeyPadScanning(void);
void segment_Set(void);
void segment_Show(uint8_t, uint8_t);
void Timer0_Init(void);
void TMR0_IRQHandler(void);
void LedMode(short);
void idleMode(void);
void alarmSetMode(void);
void countMode(void);
void pauseMode(void);
void checkMode(void);
void triggerBuzzer(void);
void delay_us(uint32_t);



int main(void) {
    System_Config();
    SetUp();
    KeyPadEnable();
    segment_Set();
    Timer0_Init();
		uint8_t key = 0;
		short flag=0;
		uint8_t index=0;
    while (1) {
			  key = KeyPadScanning();	
				delay_us(300);
				if( key == 1 ){ 
					while(key == KeyPadScanning());
					delay_us(300);
					currentState = COUNT;
					flag^=1;
					if(flag==0)
						currentState = PAUSE;
				}
					
        switch (currentState) {
            case IDLE:
								idleMode();
                if (key == 3) { // Key K3 to enter Alarm Set mode
									while(key == KeyPadScanning());
									delay_us(100);
                    alarmSetMode();
                }

                break;

            case ALARM_SET:
								alarmSetMode();
                if (PB15==0) { // GPB15 to increase alarm time
									while(!PB15);
                    alarmTime = (alarmTime + 1) % 60;
                                            
                } else if (key == 3) { // Key K3 to return to Idle mode
										while(key == KeyPadScanning());
										delay_us(100);
                    idleMode();
                }
                break;

            case COUNT:
								countMode();
                if ( flag == 0)  // Key K1 to pause
                    {	while(KeyPadScanning()==1);
											delay_us(300);
										pauseMode();} 
								else if (elapsedTime / 10 == alarmTime) {
                    triggerBuzzer();
								}
								if(KeyPadScanning() == 9)							
								{	
									while( KeyPadScanning() == 9);
									delay_us(1000);
									lapTimes[lap_num] = elapsedTime;
									lap_num++;
			        		if(lap_num==5) lap_num=0;
								}
                break;

            case PAUSE:
								pauseMode();
								index=0;
                if ( KeyPadScanning() == 1) { // Key K1 to resume
                    countMode(); } 
								else if (key == 9) { // Key K9 to reset
                    idleMode();
                } else if (key == 5) { // Key K5 to enter Check mode
									while(key == KeyPadScanning());
									delay_us(300);
                    checkMode();
                }
                break;

            case CHECK:
								segment_Show(3, index + 1);
								segment_Show(2, ((lapTimes[index]%600) / 10) / 10 ); 
								segment_Show(1, (lapTimes[index] / 10) % 10);        
								segment_Show(0,  lapTimes[index] % 10); 						
                if (PB15 == 0) { // GPB15 to rotate laps
									  while(!PB15);
							      index++;	
                } else if (key == 5) { // Key K5 to exit Check mode
									while(KeyPadScanning()==5);
									delay_us(300);
                    pauseMode();
                }
								if(index == 5) index=0;								
                break;
        }

    }
}

void System_Config(void) {
		SYS_UnlockReg(); // Unlock protected registers
    //Enable clock sources
    CLK->PWRCON |= 1<<0;
    while(!(CLK->CLKSTATUS & HXT_STATUS));  
    //PLL configuration starts
    CLK->PLLCON &= (~(1<<19));
    CLK->PLLCON &= (~(1<<16));
    CLK->PLLCON &= (~(0x01FF << 0));
    CLK->PLLCON |= PLLCON_FB_DV_VAL;	
    CLK->PLLCON &= (~(1<<18));   
    while(!(CLK->CLKSTATUS & PLL_STATUS));
    //PLL configuration ends
    //clock source selection
    CLK->CLKSEL0 &= (~(0b111<<0));
    CLK->CLKSEL0 |= 0b010; 
    //clock frequency division
    CLK->CLKDIV &= ~(0xF<<0);
    CLK->CLKDIV |= (CPUCLOCKDIVIDE-1);    
    SYS_LockReg();  // Lock protected registers	
}

//
void SetUp(void) {
		//button GPB15
		GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
		PB15 = 0;
		// Led setup
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);    
    PC12 = 1;
    PC13 = 1;
    PC14 = 1;
    PC15 = 1;
}

void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (uint32_t j = 0; j < 2; j++) { 
            __NOP(); 
        }
    }
}
void Timer0_Init(void) {
    SYS_UnlockReg();
    CLK->APBCLK |= (1 << 2); // Enable Timer0 clock
    CLK->CLKSEL1 &= ~(0b111 << 8); // Clear Timer0 clock source selection
    CLK->CLKSEL1 |= (0b010 << 8);  // Select HCLK as Timer0 clock source
    
    TIMER0->TCSR = 0; // Clear TCSR
    TIMER0->TCSR |= (1 << 26); // Reset Timer0
    TIMER0->TCSR &= ~(0xFF << 0); // No prescale
    TIMER0->TCMPR = 1200000; // Set compare value for 0.1 second
    TIMER0->TCSR |= (1 << 29); // Enable interrupt
    TIMER0->TCSR |= (0b01 << 27); // Set periodic mode
    NVIC->ISER[0] |= (1 << 8); // Enable Timer0 interrupt in NVIC
    TIMER0->TCSR |= (1 << 30); // Start Timer0
    SYS_LockReg();
}

void TMR0_IRQHandler(void) {
    // Check if the Timer0 interrupt flag is set
    if (TIMER0->TISR & (1 << 0))   
        TIMER0->TISR |= (1 << 0);   // Clear the interrupt flag

    // Handle the COUNT state
    if (currentState == COUNT) {
            elapsedTime++; // Increment elapsed time (in 1/10 seconds)
            // Check if the elapsed time matches the alarm time
            if ((elapsedTime / 10) == alarmTime) {
                //triggerBuzzer(); // Trigger the buzzer
                //idleMode();      // Return to IDLE mode after alarm
            }
    }
    
}

void segment_Set(void){//function to set GPIO mode  for 7segment
		//Set mode for PC4 to PC7 - output push-pull
		GPIO_SetMode(PC, BIT4, GPIO_MODE_OUTPUT);
		GPIO_SetMode(PC, BIT5, GPIO_MODE_OUTPUT);
		GPIO_SetMode(PC, BIT6, GPIO_MODE_OUTPUT);
		GPIO_SetMode(PC, BIT7, GPIO_MODE_OUTPUT);		
		//Set mode for PE0 to PE7 - output push-pull\\
		
	  GPIO_SetMode(PE, BIT0, GPIO_MODE_OUTPUT);		
		GPIO_SetMode(PE, BIT1, GPIO_MODE_OUTPUT);		
		GPIO_SetMode(PE, BIT2, GPIO_MODE_OUTPUT);		
		GPIO_SetMode(PE, BIT3, GPIO_MODE_OUTPUT);		
		GPIO_SetMode(PE, BIT4, GPIO_MODE_OUTPUT);		
		GPIO_SetMode(PE, BIT5, GPIO_MODE_OUTPUT);		
		GPIO_SetMode(PE, BIT6, GPIO_MODE_OUTPUT);		
		GPIO_SetMode(PE, BIT7, GPIO_MODE_OUTPUT);		
		//turn off all 7-seg
		PC->DOUT &= ~(1<<7);    //SC4
		PC->DOUT &= ~(1<<6);		//SC3
		PC->DOUT &= ~(1<<5);		//SC2
		PC->DOUT &= ~(1<<4);		//SC1		
	
}
void segment_Show(uint8_t digits, uint8_t number){//function to show digits of 7segment
	 // Set segments
  PE->DOUT = pattern[number]; // output --> number 
	   // Activate the selected digit
	switch(digits) {
		case 0: PC4=1;PC5=0;PC6=0;PC7=0; break;
		case 1: PC4=0;PC5=1;PC6=0;PC7=0; 
				if(currentState == IDLE ) 		PE1 = 1;
				else PE1 = 0;
		break;
		case 2: PC4=0;PC5=0;PC6=1;PC7=0; break;
		case 3: PC4=0;PC5=0;PC6=0;PC7=1; break;}
	delay_us(1000);
}
void LedMode(short led) { // set led of MODEs
    PC12 = 1;
    PC13 = 1;
    PC14 = 1;
    PC15 = 1;
    switch (led) {
        case 5:
            PC12 = 0;PC13 = 1;PC14 = 1;PC15 = 1; // Led5
            break;
        case 6:
            PC12 = 1;PC13 = 0;PC14 = 1;PC15 = 1; // Led6
            break;
        case 7:
            PC12 = 1;PC13 = 1;PC14 = 0;PC15 = 1; // Led7
            break;
        case 8:
            PC12 = 1;PC13 = 1;PC14 = 1;PC15 = 0; // Led8
            break;
        default:
            break;
    }
}

void idleMode(void) {
		LedMode(5);
    currentState = IDLE;
    elapsedTime = 0;
    segment_Show(0, 0);
    segment_Show(1, 0);
    segment_Show(2, 0);
    segment_Show(3, 0);
}

void alarmSetMode(void) {
		
		LedMode(8);
    currentState = ALARM_SET;
		segment_Show(1, alarmTime % 10);
    segment_Show(2, alarmTime / 10);

}	

void countMode(void) {
    LedMode(6);
    currentState = COUNT;
		TIMER0->TCSR |= (1 << 30); // Start Timer0
    // Assuming elapsedTime is already updated by Timer0 interrupt
    segment_Show(3, (elapsedTime / 600)); 		  // M
    segment_Show(2, ((elapsedTime%600) / 10) / 10 ); 
    segment_Show(1, (elapsedTime / 10) % 10);    
    segment_Show(0,  elapsedTime % 10);                      	
}

void pauseMode(void) {
    LedMode(7);
    currentState = PAUSE;
    TIMER0->TCSR &= ~(1 << 30); // Stop Timer0
    segment_Show(3, (elapsedTime / 600)); 		  // M
    segment_Show(2, ((elapsedTime%600) / 10) / 10 ); 
    segment_Show(1, (elapsedTime / 10) % 10);        
    segment_Show(0,  elapsedTime % 10);    	  
}

void checkMode(void) {
    currentState = CHECK;
   	
}

void triggerBuzzer(void) {
		PC4=1;PC5=1;PC6=1;PC7=1;
    for (int i = 0; i < 3; i++) {  			
        PB11 = 0;
        delay_us(100000);
        PB11 = 1;
        delay_us(100000);
    }
		

}
void KeyPadEnable(void){//function to set GPIO mode for key matrix 
	GPIO_SetMode(PA, BIT0, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT1, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT2, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT3, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT4, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT5, GPIO_MODE_QUASI);
}
uint8_t KeyPadScanning(void) {//function to scan the key pressed
	PA0=1; PA1=1; PA2=0; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 1;
	if (PA4==0) return 4;
	if (PA5==0) return 7;
	PA0=1; PA1=0; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 2;
	if (PA4==0) return 5;
	if (PA5==0) return 8;
	PA0=0; PA1=1; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 3;
	if (PA4==0) return 6;
	if (PA5==0) return 9;
	return 0;
}
