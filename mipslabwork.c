/* mipslabwork.c

	#################
	#  Tic-Tac-Toe  #
	#################

	Updated 2020-03-01 by Adeel Hussain & Alan Hajo

	For copyright and licensing, see file COPYING */

#include <stdio.h>
#include <stdint.h>  /* Declarations of uint_32 and the like */
#include <stdlib.h>
#include <pic32mx.h> /* Declarations of system-specific addresses etc */
#include "mipslab.h" /* Declatations for these labs */
#include <string.h>

char textstring[] = "text, more text, and even more text!";
int xTimer = 0xFF;
int oTimer = 0xFF;

int menu(void)
{
	display_string(0, "   Tic-Tac-Toe   ");
	display_string(1, "BTN 4: HowToPlay");
	display_string(2, "BTN 3: Hi-Score");
	display_string(3, "BTN 2: Start!");
	display_update();
	screen = 0;
}

int instr(void)
{
	display_string(0, "BTN 4: Left");
	display_string(1, "BTN 3: Confirm");
	display_string(2, "BTN 2: Right");
	display_string(3, "Back press BTN 1");
	display_update();
	screen = 1;
}

int board(void)
{
	display_string(0, "  TicTacToe  ");
	display_string(1, "|-|-|-|");
	display_string(2, "|-|-|-|");
	display_string(3, "|-|-|-|");
	createCursor();
	display_update();
}
/* Interrupt Service Routine */
void user_isr(void)
{
	//check flag
	if (IFS(0) & 0x100)
	{
		IFS(0) = 0; //clear flag
		chessClock();
	}
	return;
}

/* Lab-specific initialization goes here */
void labinit(void)
{
	clockTimer();
	return;
}

/* This function is called repetitively from the main program */
void labwork(void)
{
	int sw = getsw();
	int btn = getbtns();
	int btn1 = getbtn1();

	/*			GLOBAL Buttons 			*/
	if ((btn1 & 1) && screen != 4) //BTN 1
	{
		menu(); //Back to menu
		resetGame(); //Resets game
	}

	/*			Menu Buttons 		*/
	else if ((btn & 1) && screen == 0) //BTN 2
	{
		board(); //Start Game
		delay(250);
		screen = 3;
	}

	else if ((btn & 2) && screen == 0) //BTN 3
	{
		hiScore();
	}

	else if ((btn & 4) && screen == 0) //BTN 4
	{
		instr();
	}

	/*			Game Buttons			*/
	else if ((btn & 1) && screen == 3) //BTN 2
	{
		moveCursor(2);
	}
	else if ((btn & 4) && screen == 3) //BTN 4
	{
		moveCursor(1);
	}

	else if ((btn & 2) && screen == 3) //BTN 3
	{
		place();
	}

	/*		Write High Score Buttons		*/

	else if (((btn & 4) && screen == 4) && letter > 65) //BTN 4, Bokstav bak
	{
		letter--;
		textbuffer[3][initials] = letter;
		display_update();
		delay(200);
	}

	else if (((btn & 1) && screen == 4) && letter < 90) //BTN 2, Bokstav fram
	{
		letter++;
		textbuffer[3][initials] = letter;
		display_update();
		delay(200);
	}

	else if (((btn & 2) && screen == 4) && initials < 3) //BTN 3
	{
		
		delay(250);
		nameTemp[initials] = letter; //Writes to temp

		textbuffer[3][initials] = letter;
		if(initials != 2){
			textbuffer[3][initials+1] = 65;
		}
		initials++;
		display_update();
		if(initials == 3)
		{
			textbuffer[3][initials] = 0;
			resetGame();
			compareScore(calcG);
			menu();
		}
		letter = 65;
	}
}

int hiScore(void)
{
	clearScreen();
	display_string(0, "  High Score  ");

	textbuffer[1][0] = 49; //"1"
	textbuffer[1][1] = 46; //"."

	textbuffer[2][0] = 50; //"2"
	textbuffer[2][1] = 46; //"."

	//Show names
	int i;
	char sc[2] = "";
	display_update();
	if(nameScore[0] != 0){
		delay(250);
		for (i = 2; i < 5; i++)
		{
			textbuffer[1][i] = name1[i-2]; //Display name
			itoa(nameScore[0], sc);	
			textbuffer[1][6] = sc[0]; //Score
			textbuffer[1][7] = sc[1]; //Score
			display_update();
		}
	}
	if(nameScore[1] != 0){
		for (i = 2; i < 5; i++)
		{
			textbuffer[2][i] = name2[i-2]; //Display name
			itoa(nameScore[1], sc);	
			textbuffer[2][6] = sc[0]; //Score
			textbuffer[2][7] = sc[1]; //Score
			display_update();
		}
	}

	
	display_string(3, "Back press BTN 1");
	display_update();
	screen = 2;
}

int writeHiScore(void)
{
	screen = 4;
	clearScreen();

	//Show who won
	display_string(0, "Winner:     P: "); //Base
	if (win == 1) //X-win
	{
		textbuffer[0][8] = 88; //X
		timeLeft(xTimer, 0, 14);
	}
	else if (win == 2) //O-win
	{
		textbuffer[0][8] = 79; //O
		timeLeft(oTimer, 0, 14);
	}
	display_string(2, "Enter name:");

	textbuffer[3][0] = 65; //A
	display_update();
}