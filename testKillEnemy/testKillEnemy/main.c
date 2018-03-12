
#include "timer.h"
#include "scheduler.h"
//#include "io.c"
#include "SPI_LCD.c"

// ---------------------------------------------------------------------------
// Shared Variables

	unsigned char x = 80;
	unsigned char y = 65;

	int16_t U_D;
	int16_t L_R;

//Initialize the ADC
void ADC_init()
{
	ADMUX = (1<<REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

//Read from the ADC channel ch
uint16_t ReadADC(uint8_t ch)
{
	//Select ADC Channel
	ch &= 0x07;
	ADMUX = (ADMUX & 0xF8) | ch;

	//Start Single conversion
	ADCSRA |= (1 << ADSC);

	//Wait for conversion to complete
	while(ADCSRA & (1 << ADSC));

	//Clear ADIF
	ADCSRA |= (1<<ADIF);
	return (ADC);
}
//handle up-down movement
void moveU_D(int16_t U_D)
{
	unsigned char tmp = y;
	if(U_D > 0)
	{
		if(tmp >= 10)
		{
			tmp = (tmp == 7) ? 0 : tmp-5;
		}
	}
	else if(U_D < 0)
	{
		if(tmp <= 128)
		{
			tmp = (tmp == 0) ? 7 : tmp+5;
		}
	}
	y = tmp;
}

//handle left-right movement
void moveL_R(int16_t L_R)
{
	unsigned char tmp = x;
	if(L_R < 0)
	{
		if(tmp >= 10)
		{
			tmp = (tmp == 4) ? 0 : tmp-5;
		}
	}
	else if(L_R > 0)
	{
		if(tmp <= 160)
		{
			tmp = (tmp == 0) ? 4 : tmp+5;
		}
	}
	x = tmp;
}

//determines horizontal cursor movement from joystick input
enum CPH_States {CPH_Stay, CPH_Move};
int TickFct_CursorPos_H(int state)
{
	L_R = ReadADC(2);
	L_R -= 512;
	switch(state)//Transitions
	{
		case CPH_Stay:
		if((L_R <= 250) && (L_R > -249))
		{
			state = CPH_Stay;
		}
		else
		{
			state = CPH_Move;
		}
		break;
		case CPH_Move:
		if((L_R <= 250) && (L_R > -249))
		{
			state = CPH_Stay;
		}
		else
		{
			state = CPH_Move;
		}
		break;
		default:
		state = CPH_Stay;
		break;
	}
	switch(state)//Actions
	{
		case CPH_Stay:
		//cursor_blink = 0;
		break;
		case CPH_Move:
		moveL_R(L_R);
		//cursor_blink = 1;
		//cursor_on = 1;
		break;
	}
	return state;
}

//determines vertical cursor movement from joystick input
enum CPV_States {CPV_Stay, CPV_Move};
int TickFct_CursorPos_V(int state)
{
	U_D = ReadADC(3);
	U_D -= 512;
	switch(state)//Transitions
	{
		case CPV_Stay:
		if((U_D <= 250) && (U_D > -249))
		{
			state = CPV_Stay;
		}
		else
		{
			state = CPV_Move;
		}
		break;
		case CPV_Move:
		if((U_D <= 250) && (U_D > -249))
		{
			state = CPV_Stay;
		}
		else
		{
			state = CPV_Move;
		}
		break;
		default:
		state = CPV_Stay;
		break;
	}
	switch(state)//Actions
	{
		case CPV_Stay:
		//cursor_blink = 0;
		break;
		case CPV_Move:
		moveU_D(U_D);
		//cursor_blink = 1;
		//cursor_on = 1;
		break;
	}
	return state;
}

// Moves character
enum Character_States{Move};
int Tick3(int state)
{
	switch(state)
	{
		case Move:
		state = Move;
		break;
		
		default:
		state = Move;
		break;
	}
	switch(state)
	{
		case Move:
		FillCircle (x, y, 10, BLUE);
		msDelay(100);
		FillCircle (x, y, 10, BLACK);
		break;
	}
	return state;
}

// Handles gems
enum Gem_States{there, not_there};
int Tick4(int state)
{
	switch(state) // transitions
	{
		case there:
		state = there;
		break;
		
		default:
		state = there;
		break;
	}
	switch(state) // actions
	{
		case there:
		FillCircle (100, 105, 8, GREEN); // bottom right
		FillCircle (50, 20, 8, GREEN); // top left
		FillCircle (50, 105, 8, GREEN); // bottom left
		FillCircle (100, 20, 8, GREEN); // top right
		break;
		
		default:
		break;
	}
	return state;
}

// Enemies
enum Enemy_States{alive, dead1, dead2, dead3, dead4, attack};
int Tick5(int state)
{
	switch(state) // transitions
	{
		case alive:
		if((x <= 30 && x >= 5) && (y <= 30 && y >= 10))
		{
			state = dead1;
		}
		else
		{
			state = alive;
		}
		break;
		
		case dead1:
		state = dead1;
		break;
		
		default:
		state = alive;
		break;
	}
	switch(state) // actions
	{
		case alive:
		FillCircle (20, 20, 7, RED); // top left
		FillCircle (20, 105, 7, RED); // bottom left
		FillCircle (130, 20, 7, RED); // top right
		FillCircle (130, 105, 7, RED); // bottom right
		break;
		
		case dead1:
		FillCircle (20, 20, 7, BLACK); // top left
		break;
		
		default:
		break;
	}
	return state;
}
// ---------------------------------------------------------------------------
// MAIN PROGRAM
int main()
{
	DDRA = 0x83; PORTA = 0x7C;
	
	// Period for the tasks
	unsigned long int SMTick1_calc = 50;
	unsigned long int SMTick2_calc = 50;
	unsigned long int SMTick3_calc = 50;
	unsigned long int SMTick4_calc = 50;
	unsigned long int SMTick5_calc = 50;
	
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);
	tmpGCD = findGCD(tmpGCD, SMTick3_calc);
	tmpGCD = findGCD(tmpGCD, SMTick4_calc);
	tmpGCD = findGCD(tmpGCD, SMTick5_calc);
	
	unsigned long int GCD = tmpGCD;
	
	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	unsigned long int SMTick3_period = SMTick3_calc/GCD;
	unsigned long int SMTick4_period = SMTick4_calc/GCD;
	unsigned long int SMTick5_period = SMTick5_calc/GCD;

	//Declare an array of tasks
	static task task1, task2, task3, task4, task5;
	task *tasks[] = { &task1, &task2, &task3, &task4, &task5 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &TickFct_CursorPos_H;//Function pointer for the tick.

	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = SMTick2_period;//Task Period.
	task2.elapsedTime = SMTick2_period;//Task current elapsed time.
	task2.TickFct = &TickFct_CursorPos_V;//Function pointer for the tick.
	
	// Task 3
	task3.state = -1;//Task initial state.
	task3.period = SMTick3_period;//Task Period.
	task3.elapsedTime = SMTick3_period; // Task current elasped time.
	task3.TickFct = &Tick3; // Function pointer for the tick.
	
	// Task 4
	task4.state = -1;//Task initial state.
	task4.period = SMTick4_period;//Task Period.
	task4.elapsedTime = SMTick4_period; // Task current elasped time.
	task4.TickFct = &Tick4; // Function pointer for the tick.
	
	// Task 5
	task5.state = -1;//Task initial state.
	task5.period = SMTick5_period;//Task Period.
	task5.elapsedTime = SMTick5_period; // Task current elasped time.
	task5.TickFct = &Tick5; // Function pointer for the tick.

	//Set Initial Conditions
	TimerSet(GCD);
	TimerOn();
	ADC_init();
	SetupPorts(); // use PortB for LCD interface
	OpenSPI(); // start communication to TFT
	InitDisplay(); // initialize TFT controller
	ClearScreen();
	SetOrientation(90);
 
	//CV_state = CPV_Stay;
	//CP_state = CPH_Stay;
 /*
  while (1) {
	  TickFct_CursorPos_H();
	  TickFct_CursorPos_V();
	  FillCircle (x, y, 15, RED);
	  msDelay(100);
	  FillCircle (x, y, 15, BLACK);
	  while (!TimerFlag){}   // Wait for timer period
	  TimerFlag = 0;         // Lower flag raised by timer
  }
  */
 unsigned short i; // Scheduler for-loop iterator
 while(1) {
	 // Scheduler code
	 for ( i = 0; i < numTasks; i++ ) {
		 // Task is ready to tick
		 if ( tasks[i]->elapsedTime == tasks[i]->period ) {
			 // Setting next state for task
			 tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
			 // Reset the elapsed time for next tick.
			 tasks[i]->elapsedTime = 0;
		 }
		 tasks[i]->elapsedTime += 1;
	 }
	 while(!TimerFlag);
	 TimerFlag = 0;
 }
  
 CloseSPI(); // close communication with TFT
 return 0;
}