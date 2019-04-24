
#include "timer.h"
#include "scheduler.h"
#include "SPI_LCD.c"

// ---------------------------------------------------------------------------

unsigned char x = 80; // shared variable for main character x axis
unsigned char y = 65; // shared variable for main character y axis

int16_t upDown;  
int16_t leftRight;

//Initialize ADC
void ADC_init()
{
	ADMUX = (1<<REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

//Read from ADC channel
uint16_t ReadADC(uint8_t data)
{
	data &= 0x07;
	ADMUX = (ADMUX & 0xF8) | data;

	ADCSRA |= (1 << ADSC);

	while(ADCSRA & (1 << ADSC));

	ADCSRA |= (1<<ADIF);
	return (ADC);
}

//Up and down movement determination
void move1(int16_t upDown)
{
	unsigned char temp = y;
	if(upDown > 0)
	{
		if(temp >= 10)
		{
			temp = (temp == 7) ? 0 : temp-1;
		}
	}
	else if(upDown < 0)
	{
		if(temp <= 128)
		{
			temp = (temp == 0) ? 7 : temp+1;
		}
	}
	y = temp;
}

//Left and right movement determination
void move2(int16_t leftRight)
{
	unsigned char temp = x;
	if(leftRight < 0)
	{
		if(temp >= 10)
		{
			temp = (temp == 4) ? 0 : temp-1;
		}
	}
	else if(leftRight > 0)
	{
		if(temp <= 160)
		{
			temp = (temp == 0) ? 4 : temp+1;
		}
	}
	x = temp;
}

//Joystick left and right
enum LeftRight_States {still1, go1};
int Tick1(int state)
{
	leftRight = ReadADC(2);
	leftRight = leftRight - 512;
	
	switch(state)//Transitions
	{
		case still1:
		if((leftRight <= 250) && (leftRight > -249))
		{
			state = still1;
		}
		else
		{
			state = go1;
		}
		break;
		
		case go1:
		if((leftRight <= 250) && (leftRight > -249))
		{
			state = still1;
		}
		else
		{
			state = go1;
		}
		break;
		
		default:
		state = still1;
		break;
	}
	switch(state)//Actions
	{
		case still1:
		break;
		
		case go1:
		move2(leftRight);
		break;
	}
	return state;
}

//Joystick up and down
enum UpDown_States {still2, go2};
int Tick2(int state)
{
	upDown = ReadADC(3);
	upDown = upDown - 512;
	
	switch(state)//Transitions
	{
		case still2:
		if((upDown <= 250) && (upDown > -249))
		{
			state = still2;
		}
		else
		{
			state = go2;
		}
		break;
		
		case go2:
		if((upDown <= 250) && (upDown > -249))
		{
			state = still2;
		}
		else
		{
			state = go2;
		}
		break;
		
		default:
		state = still2;
		break;
	}
	switch(state)//Actions
	{
		case still2:
		break;
		
		case go2:
		move1(upDown);
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
		Circle(x, y, 11, BLACK);
		Circle(x, y, 12, BLACK);
		break;
	}
	return state;
}

// Handles gems and game display
enum Gem_States{welcome, there, gameOver};
	unsigned char gem1 = 0; // shared variable for handling top left gem on LCD
	unsigned char gem2 = 0; // shared variable for handling bottom left gem on LCD
	unsigned char gem3 = 0; // shared variable for handling top right gem on LCD
	unsigned char gem4 = 0; // shared variable for handling bottom right gem on LCD
	unsigned char score = 0; // shared variable for displaying score
	unsigned char display = 0; // shared variable for coordinating LCD characters 
int Tick4(int state)
{
	unsigned char press = ~PINC & 0x01;
	
	switch(state) // transitions
	{
		case welcome:
		if(press)
		{
			ClearScreen();
			state = there;
		}
		else
		{
			state = welcome;
		}
		break;
		
		case there:
		if(gem1 == 1 && gem2 == 1 && gem3 == 1 && gem4 == 1)
		{
			ClearScreen();
			state = gameOver;
		}
		else if(press)
		{
			ClearScreen();
			state = gameOver;
		}
		else
		{
			state = there;
		}
		break;
		
		case gameOver:
		if(press)
		{
			ClearScreen();
			state = welcome;
		}
		else
		{
			state = gameOver;
		}
		break;
		
		default:
		state = welcome;
		break;
	}
	switch(state) // actions
	{
		case welcome:
		display = 1;
		gem1 = 0;
		gem2 = 0;
		gem3 = 0;
		gem4 = 0;
		score = 0;
		
		OpenSPI(); // start communication to TFT
		char *str3 = "WELCOME!"; // text to display
		GotoXY(9,3); // position text cursor
		WriteString(str3,MAGENTA); // display text
		GotoXY(5,10);
		char *str4 = "Press the start";
		WriteString(str4,MAGENTA);
		GotoXY(9,12);
		char *str5 = "button";
		WriteString(str5,MAGENTA);
		break;
		
		case there:
		display = 2;
		
		gem4 != 1 ? FillEllipse (100, 105, 16, 25, LIME) : FillEllipse (100, 105, 16, 25, BLACK); // bottom right
		
		gem1 != 1 ?	FillEllipse (50, 20, 16,25, LIME) : FillEllipse (50, 20, 16,25, BLACK); // top left
		
		gem2 != 1 ? FillEllipse (50, 105, 16,25, LIME) : FillEllipse (50, 105, 16,25, BLACK); // bottom left
		
		gem3 != 1 ? FillEllipse (100, 20, 16,25, LIME) : FillEllipse (100, 20, 16,25, BLACK); // top right
		
		break;
		
		case gameOver:
		display = 3;
		OpenSPI(); // start communication to TFT
		char *str1 = "GAME OVER"; // text to display
		GotoXY(9,4); // position text cursor
		WriteString(str1,YELLOW); // display text
		GotoXY(11,6);
		char *str2 = "SCORE:";
		WriteString(str2,YELLOW);
		GotoXY(13,8);
		WriteInt(score);
		break;

		default:
		break;
	}
	return state;
}

// Enemy 1 - top left
enum Enemy1_States{alive1, dead1, attack1, wait1};
	unsigned char E1x = 1; // shared variable for enemy 1 x axis
	unsigned char E1y = 20; // shared variable for enemy 1 y axis
	unsigned char k1 = 0; // shared variable for enemy respawn rate
int Tick5(int state)
{
	switch(state) // transitions
	{
		case alive1:
		if((x <= E1x+8 && x >= E1x-8) && (y <= E1y+8 && y >= E1y-8))
		{
			score = score + 1;
			state = dead1;
		}
		else if(E1x+15 == 50)
		{
			state = attack1;
		}
		else
		{
			state = alive1;
		}
		break;
		
		case dead1:
		if(gem1 == 0)
		{
			state = wait1;
		}
		else
		{
			state = dead1;
		}
		break;
		
		case wait1:
		if(k1 == 20)
		{
			state = alive1;
		}
		else
		{
			state = wait1;
		}
		break;
		
		case attack1:
		state = dead1;
		break;
		
		default:
		state = alive1;
		break;
	}
	switch(state) // actions
	{
		case alive1:
		if(display == 2)
		{
			FillCircle (E1x, E1y, 7, RED); // top left
			Circle(E1x, E1y, 8, BLACK);
			E1x = E1x + 1;
			k1 = 0;
		}
		else
		{
			FillCircle (E1x, E1y, 8, BLACK); // top left
			E1x = 1;
		}
		break;
		
		case dead1:
		FillCircle (E1x, E1y, 8, BLACK); // top left
		E1x = 1;
		break;
		
		case wait1:
		k1 = k1 + 1;
		break;
		
		case attack1:
		gem1 = 1;
		break;
		
		default:
		break;
	}
	return state;
}

// Enemy 2 - bottom left
enum Enemy2_States{alive2, dead2, attack2, wait2};
unsigned char E2x = 1; // shared variable for enemy 2 x axis
unsigned char E2y = 105; // shared variable for enemy 2 y axis
unsigned char k2 = 0; // shared variable for enemy 2 respawn rate
int Tick6(int state)
{
	switch(state) // transitions
	{
		case alive2:
		if((x <= E2x+8 && x >= E2x-8) && (y <= E2y+8 && y >= E2y-8))
		{
			score = score + 1;
			state = dead2;
		}
		else if(E2x+15 == 50)
		{
			state = attack2;
		}
		else
		{
			state = alive2;
		}
		break;
		
		case dead2:
		if(gem2 == 0)
		{
			state = wait2;
		}
		else
		{
			state = dead2;
		}
		break;
		
		case wait2:
		if(k2 == 15)
		{
			state = alive2;
		}
		else
		{
			state = wait2;
		}
		break;
		
		case attack2:
		state = dead2;
		break;
		
		default:
		state = wait2;
		break;
	}
	switch(state) // actions
	{
		case alive2:
		if(display == 2)
		{
			FillCircle (E2x, E2y, 7, RED); // top left
			Circle(E2x, E2y, 8, BLACK);
			E2x = E2x + 1;
			k2 = 0;
		}
		else
		{
			FillCircle (E2x, E2y, 8, BLACK); // top left
			E2x = 1;
		}
		break;
		
		case dead2:
		FillCircle (E2x, E2y, 8, BLACK); // top left
		E2x = 1;
		break;
		
		case wait2:
		k2 = k2 + 1;
		break;
		
		case attack2:
		gem2 = 1;
		break;
		
		default:
		break;
	}
	return state;
}

// Enemy 3 - top right
enum Enemy3_States{alive3, dead3, attack3, wait3};
unsigned char E3x = 160; // shared variable for enemy 3 x axis
unsigned char E3y = 20; // shared variable for enemy 3 y axis
unsigned char k3 = 0; // shared variable for enemy 3 respawn rate
int Tick7(int state)
{
	switch(state) // transitions
	{
		case alive3:
		if((x <= E3x+8 && x >= E3x-8) && (y <= E3y+8 && y >= E3y-8))
		{
			score = score + 1;
			state = dead3;
		}
		else if(E3x-15 == 100)
		{
			state = attack3;
		}
		else
		{
			state = alive3;
		}
		break;
		
		case dead3:
		if(gem3 == 0)
		{
			state = wait3;
		}
		else
		{
			state = dead3;
		}
		break;
		
		case wait3:
		if(k3 == 5)
		{
			state = alive3;
		}
		else
		{
			state = wait3;
		}
		break;
		
		case attack3:
		state = dead3;
		break;
		
		default:
		state = alive3;
		break;
	}
	switch(state) // actions
	{
		case alive3:
		if(display == 2)
		{
			FillCircle (E3x, E3y, 7, RED); // top left
			Circle(E3x, E3y, 8, BLACK);
			E3x = E3x - 1;
			k3 = 0;
		}
		else
		{
			FillCircle (E3x, E3y, 8, BLACK); // top left
			E3x = 160;
		}
		break;
		
		case dead3:
		FillCircle (E3x, E3y, 8, BLACK); // top left
		E3x = 160;
		break;
		
		case wait3:
		k3 = k3 + 1;
		break;
		
		case attack3:
		gem3 = 1;
		break;
		
		default:
		break;
	}
	return state;
}

// Enemy 4 - bottom right
enum Enemy4_States{alive4, dead4, attack4, wait4};
unsigned char E4x = 160; // shared variable for enemy 4 x axis
unsigned char E4y = 105; // shared variable for enemy 4 y axis
unsigned char k4 = 0; // shared variable for enemy 4 respawn
int Tick8(int state)
{
	switch(state) // transitions
	{
		case alive4:
		if((x <= E4x+8 && x >= E4x-8) && (y <= E4y+8 && y >= E4y-8))
		{
			score = score + 1;
			state = dead4;
		}
		else if(E4x-15 == 100)
		{
			state = attack4;
		}
		else
		{
			state = alive4;
		}
		break;
		
		case dead4:
		if(gem4 == 0)
		{
			state = wait4;
		}
		else
		{
			state = dead4;
		}
		break;
		
		case wait4:
		if(k4 == 35)
		{
			state = alive4;
		}
		else
		{
			state = wait4;
		}
		break;
		
		case attack4:
		state = dead4;
		break;
		
		default:
		state = wait4;
		break;
	}
	switch(state) // actions
	{
		case alive4:
		if(display == 2)
		{
			FillCircle (E4x, E4y, 7, RED); // top left
			Circle(E4x, E4y, 8, BLACK);
			E4x = E4x - 1;
			k4 = 0;
		}
		else
		{
			FillCircle (E4x, E4y, 8, BLACK); // top left
			E4x = 160;
		}
		break;
		
		case dead4:
		FillCircle (E4x, E4y, 8, BLACK); // top left
		E4x = 160;
		break;
		
		case wait4:
		k4 = k4 + 1;
		break;
		
		case attack4:
		gem4 = 1;
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
	DDRC = 0x00; PORTC = 0xFF;
	
	// Period for the tasks
	unsigned long int SMTick1_calc = 5;
	unsigned long int SMTick2_calc = 5;
	unsigned long int SMTick3_calc = 5;
	unsigned long int SMTick4_calc = 50;
	unsigned long int SMTick5_calc = 30; // top left
	unsigned long int SMTick6_calc = 35; // bottom left
	unsigned long int SMTick7_calc = 50; // top right
	unsigned long int SMTick8_calc = 25; // bottom right
	
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);
	tmpGCD = findGCD(tmpGCD, SMTick3_calc);
	tmpGCD = findGCD(tmpGCD, SMTick4_calc);
	tmpGCD = findGCD(tmpGCD, SMTick5_calc);
	tmpGCD = findGCD(tmpGCD, SMTick6_calc);
	tmpGCD = findGCD(tmpGCD, SMTick7_calc);
	tmpGCD = findGCD(tmpGCD, SMTick8_calc);
	
	unsigned long int GCD = tmpGCD;
	
	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	unsigned long int SMTick3_period = SMTick3_calc/GCD;
	unsigned long int SMTick4_period = SMTick4_calc/GCD;
	unsigned long int SMTick5_period = SMTick5_calc/GCD;
	unsigned long int SMTick6_period = SMTick6_calc/GCD;
	unsigned long int SMTick7_period = SMTick7_calc/GCD;
	unsigned long int SMTick8_period = SMTick8_calc/GCD;

	//Declare an array of tasks
	static task task1, task2, task3, task4, task5, task6, task7, task8;
	task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6, &task7, &task8 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &Tick1;//Function pointer for the tick.

	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = SMTick2_period;//Task Period.
	task2.elapsedTime = SMTick2_period;//Task current elapsed time.
	task2.TickFct = &Tick2;//Function pointer for the tick.
	
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
	
	// Task 6
	task6.state = -1;//Task initial state.
	task6.period = SMTick6_period;//Task Period.
	task6.elapsedTime = SMTick6_period; // Task current elasped time.
	task6.TickFct = &Tick6; // Function pointer for the tick.
	
	// Task 7
	task7.state = -1;//Task initial state.
	task7.period = SMTick7_period;//Task Period.
	task7.elapsedTime = SMTick7_period; // Task current elasped time.
	task7.TickFct = &Tick7; // Function pointer for the tick.
	
	// Task 8
	task8.state = -1;//Task initial state.
	task8.period = SMTick8_period;//Task Period.
	task8.elapsedTime = SMTick8_period; // Task current elasped time.
	task8.TickFct = &Tick8; // Function pointer for the tick.

	//Set Initial Conditions
	TimerSet(GCD);
	TimerOn();
	ADC_init();
	SetupPorts(); // use PortB for LCD interface
	OpenSPI(); // start communication to TFT
	InitDisplay(); // initialize TFT controller
	ClearScreen(); 
	SetOrientation(90); // using LCD screen in landscape mode
 
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