/* mipslabfunc.c
   This file written 2015 by F Lundevall
	
	#################
	#  Tic-Tac-Toe  #
	#################

	Updated 2020-03-01 by Adeel Hussain & Alan Hajo

	For copyright and licensing, see file COPYING */

#include <stdio.h>
#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */
#include <string.h> 
#include <stdlib.h>

/* Declare a helper function which is local to this file */
static void num32asc( char * s, int ); 

#define DISPLAY_CHANGE_TO_COMMAND_MODE (PORTFCLR = 0x10)
#define DISPLAY_CHANGE_TO_DATA_MODE (PORTFSET = 0x10)

#define DISPLAY_ACTIVATE_RESET (PORTGCLR = 0x200)
#define DISPLAY_DO_NOT_RESET (PORTGSET = 0x200)

#define DISPLAY_ACTIVATE_VDD (PORTFCLR = 0x40)
#define DISPLAY_ACTIVATE_VBAT (PORTFCLR = 0x20)

#define DISPLAY_TURN_OFF_VDD (PORTFSET = 0x40)
#define DISPLAY_TURN_OFF_VBAT (PORTFSET = 0x20)

#define TMR2PERIOD ((80000000 / 256) / 10)


int currY;
int currX;
char temp;

int getsw( void )
{

  return (PORTD >> 8) & 0x000F; //Shiftar 8 bitar till höger och maskerar resten

}

int getbtns(void) {

	return (PORTD >> 5) & 0x0007; //Shiftar bitarna 5 steg till höger och maskerar resten
}

int getbtn1(void)
{
    return (PORTF >> 1) & 0x01;   //Masks all bits except bit 1
}

/* quicksleep:
   A simple function to create a small delay.
   Very inefficient use of computing resources,
   but very handy in some special cases. */
void quicksleep(int cyc) {
	int i;
	for(i = cyc; i > 0; i--);
}

/* tick:
   Add 1 to time in memory, at location pointed to by parameter.
   Time is stored as 4 pairs of 2 NBCD-digits.
   1st pair (most significant byte) counts days.
   2nd pair counts hours.
   3rd pair counts minutes.
   4th pair (least significant byte) counts seconds.
   In most labs, only the 3rd and 4th pairs are used. */
void tick( unsigned int * timep )
{
  /* Get current value, store locally */
  register unsigned int t = * timep;
  t += 1; /* Increment local copy */
  
  /* If result was not a valid BCD-coded time, adjust now */

  if( (t & 0x0000000f) >= 0x0000000a ) t += 0x00000006;
  if( (t & 0x000000f0) >= 0x00000060 ) t += 0x000000a0;
  /* Seconds are now OK */

  if( (t & 0x00000f00) >= 0x00000a00 ) t += 0x00000600;
  if( (t & 0x0000f000) >= 0x00006000 ) t += 0x0000a000;
  /* Minutes are now OK */

  if( (t & 0x000f0000) >= 0x000a0000 ) t += 0x00060000;
  if( (t & 0x00ff0000) >= 0x00240000 ) t += 0x00dc0000;
  /* Hours are now OK */

  if( (t & 0x0f000000) >= 0x0a000000 ) t += 0x06000000;
  if( (t & 0xf0000000) >= 0xa0000000 ) t = 0;
  /* Days are now OK */

  * timep = t; /* Store new value */
}

/* display_debug
   A function to help debugging.

   After calling display_debug,
   the two middle lines of the display show
   an address and its current contents.

   There's one parameter: the address to read and display.

   Note: When you use this function, you should comment out any
   repeated calls to display_image; display_image overwrites
   about half of the digits shown by display_debug.
*/   
void display_debug( volatile int * const addr )
{
  display_string( 1, "Addr" );
  display_string( 2, "Data" );
  num32asc( &textbuffer[1][6], (int) addr );
  num32asc( &textbuffer[2][6], *addr );
  display_update();
}

uint8_t spi_send_recv(uint8_t data) {
	while(!(SPI2STAT & 0x08));
	SPI2BUF = data;
	while(!(SPI2STAT & 1));
	return SPI2BUF;
}

void display_init(void) {
        DISPLAY_CHANGE_TO_COMMAND_MODE;
	quicksleep(10);
	DISPLAY_ACTIVATE_VDD;
	quicksleep(1000000);
	
	spi_send_recv(0xAE);
	DISPLAY_ACTIVATE_RESET;
	quicksleep(10);
	DISPLAY_DO_NOT_RESET;
	quicksleep(10);
	
	spi_send_recv(0x8D);
	spi_send_recv(0x14);
	
	spi_send_recv(0xD9);
	spi_send_recv(0xF1);
	
	DISPLAY_ACTIVATE_VBAT;
	quicksleep(10000000);
	
	spi_send_recv(0xA1);
	spi_send_recv(0xC8);
	
	spi_send_recv(0xDA);
	spi_send_recv(0x20);
	
	spi_send_recv(0xAF);
}

void display_string(int line, char *s) {
	int i;
	if(line < 0 || line >= 4)
		return;
	if(!s)
		return;
	
	for(i = 0; i < 16; i++)
		if(*s) {
			textbuffer[line][i] = *s;
			s++;
		} else
			textbuffer[line][i] = ' ';
}

void display_update(void) {
	int i, j, k;
	int c;
	for(i = 0; i < 4; i++) {
		DISPLAY_CHANGE_TO_COMMAND_MODE;
		spi_send_recv(0x22);
		spi_send_recv(i);
		
		spi_send_recv(0x0);
		spi_send_recv(0x10);
		
		DISPLAY_CHANGE_TO_DATA_MODE;
		
		for(j = 0; j < 16; j++) {
			c = textbuffer[i][j];
			if(c & 0x80)
				continue;
			
			for(k = 0; k < 8; k++)
				spi_send_recv(font[c*8 + k]);
		}
	}
}

/* Helper function, local to this file.
   Converts a number to hexadecimal ASCII digits. */
static void num32asc( char * s, int n ) 
{
  int i;
  for( i = 28; i >= 0; i -= 4 )
    *s++ = "0123456789ABCDEF"[ (n >> i) & 15 ];
}


/*
#############################
PROJECT FUNCTIONS
############################
*/
int chessClock()
{
	if (timerStart == 1)
		{
			if (turn == 1)
			{
				PORTE = xTimer;
				timeoutcountX++;
				if (timeoutcountX == 40)
				{ 
					timeoutcountX = 0;
					xTimer -= 0x1;
					xTimer = (xTimer / 2);
					PORTE = xTimer;
				}
				if (xTimer == 0x0)
				{
					win = 2;
					timerStart = 0;
					initWin();
				}
			}
			if (turn == 2)
			{
				PORTE = oTimer;
				timeoutcountO++;
				if (timeoutcountO == 40)
				{
					timeoutcountO = 0;
					oTimer -= 0x1;
					oTimer = (oTimer / 2);
					PORTE = oTimer;
				}
				if (oTimer == 0x0)
				{
					win = 1;
					timerStart = 0;
					initWin();
				}
			}
		}
}

int clockTimer()
{
	
	TRISECLR = 0xFF; // Set first 8 bits to zero (sets them as output pins for LED)

	TRISFCLR = 0x1; //Initialize BTN 1
	// Initialize port D, set bits 11-5 as inputs. SW1-4 & BTN2-4
	TRISD = TRISD & 0x0fe0;

	PR2 = TMR2PERIOD;
	T2CON = 0x0;	   // clearing the clock
	T2CONSET = 0x70;   // setting the prescale
	TMR2 = 0;		   // reset timer to 0
	T2CONSET = 0x8000; // turn timer on, set bit 15 to 1

	// configuring the priority level
	IPC(2) = 7;
	// enabling bit 8 for the interupt
	IEC(0) = (1 << 8);

	// calling interupt from labwork.S
	enable_interrupt();
}

int compareScore(int newScore) 
{
	int c;
  if(newScore > nameScore[0])
	{
		for (c = 0; c < 3; c++)
		{
			name2[c] = name1[c];
		}

		nameScore[1] = nameScore[0];
		
		for (c = 0; c < 3; c++)
		{
			name1[c] = nameTemp[c];
			delay(250);
		}

		nameScore[0] = newScore;
	}
	else if(newScore < nameScore[0])
	{
		if(newScore < nameScore[1])
		{
			return;
		}
		else
		{
			nameScore[1] = newScore;
			for (c = 0; c < 3; c++)
			{
				name2[c] = nameTemp[c];
			}
		}
	}
}

int resetGame(){
	win = 0;
	count = 0;
	resetBoardArr();
	xTimer = 0xFF;
	oTimer = 0xFF;
	currX = 0;
	currY = 0;
	timerStart = 0;
	turn = 1;
	initials = 0;
	letter = 65;
}

//https://en.wikibooks.org/wiki/C_Programming/stdlib.h/itoa
 void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

 void itoa(int n, char s[])
 {
     int i, sign;
 
     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }


int timeLeft(int hexad, int x, int y) { //hexad = 0xFF (int)
	hexToStr(hexad); 					      //0xFF -> "0xFF" (str)
	char *res = out; 					
	hexToBin(res);						      //"0xFF" (str) -> "11111111"
	int calc = countOnes(bin) *4;		//8*4 (int)
	itoa(calc, timeSc);					    //20 (int) -> "20" (str)
  	int b;
	for(b = 0; b < 2; b++){
		textbuffer[x][y+b] = timeSc[b];
	}
	calcG = calc;
}

int hexToStr(int hexa){
	char buff[6];
	time2string(buff, hexa);
	out[0] = buff[3];
	out[1] = buff[4];
}

//https://codeforwin.org/2015/08/c-program-to-convert-hexadecimal-to-binary-number-system.html
int hexToBin(char hex[]){ //0xFF
	int i;
	for (i = 0; i < 9; i++)
	{
		bin[i] = 0;
	}
	
	for(i=0; hex[i]!='\0'; i++)
    {
        switch(hex[i])
        {
            case '0':
                strcat(bin, "0000");
                break;
            case '1':
                strcat(bin, "0001");
                break;
            case '2':
                strcat(bin, "0010");
                break;
            case '3':
                strcat(bin, "0011");
                break;
            case '4':
                strcat(bin, "0100");
                break;
            case '5':
                strcat(bin, "0101");
                break;
            case '6':
                strcat(bin, "0110");
                break;
            case '7':
                strcat(bin, "0111");
                break;
            case '8':
                strcat(bin, "1000");
                break;
            case '9':
                strcat(bin, "1001");
                break;
            case 'a':
            case 'A':
                strcat(bin, "1010");
                break;
            case 'b':
            case 'B':
                strcat(bin, "1011");
                break;
            case 'c':
            case 'C':
                strcat(bin, "1100");
                break;
            case 'd':
            case 'D':
                strcat(bin, "1101");
                break;
            case 'e':
            case 'E':
                strcat(bin, "1110");
                break;
            case 'f':
            case 'F':
                strcat(bin, "1111");
                break;
        }
    }
}

int countOnes(char binaries[]){
	int ones = 0;
	int i;
	for(i = 0; i < 8; i++){
		if(binaries[i] == '1'){
			ones++;
		}
	}
	return ones;
}

int resetBoardArr(){
	int i, j;
	for(i = 0; i < 3; i++) {
		for(j = 0; j < 7; j++) {
      		boardArr[i][j] = resetArr[i][j];
		}
   }
}

int clearScreen(){
  int i;
  for (i = 0; i < 4; i++)
  {
    display_string(i, " ");
  }
  	display_update();
}

int winExists(void) {
  if(win == 1 || win == 2)
  {
    return 1;
  }
  else if(win == 3){
	return 2;
  }
  else
  {
    return 0;
  }
}

int initWin(){
	if(winExists() == 1){
    	timerStart = 0;
		screen = 4;
		delay(250);
		writeHiScore();
	}
}

int checkWin(void)
{
  int p;

	for(p = 0; p < 3; p++) //Kollar vinst vågrätt
	{
		if((boardArr[p][1] == boardArr[p][3]) && (boardArr[p][3] == boardArr[p][5]))
		{
			if(boardArr[p][1] == 88)
			{
				display_string(0, "X WINS!");
				display_update();
				win = 1;
				initWin();
			}
			else if (boardArr[p][1] == 79)
			{
				display_string(0, "O WINS!");
				display_update();
				win = 2;
				initWin();
			}
		}
	}

    for(p = 1; p <= 5; p+=2) //Kollar vinst lodrätt
    {
      if(boardArr[0][p] == boardArr[1][p] && boardArr[1][p] == boardArr[2][p])
      {
        if(boardArr[0][p] == 88)
        {
          display_string(0, "X WINS!");
          display_update();
          win = 1;
		  initWin();
        }
        else if (boardArr[0][p] == 79)
        {
          display_string(0, "O WINS!");
          display_update();
          win = 2;
		  initWin();
        }
      }
    }
    if (((boardArr[0][1] == boardArr[1][3]) && (boardArr[0][1] == boardArr[2][5])) ||
        ((boardArr[0][5] == boardArr[1][3]) && (boardArr[0][5] == boardArr[2][1]))) // Kollar vinst diagonalt
    {
      if(boardArr[1][3] == 88)
      {
        display_string(0, "X WINS!");
        display_update();
        win = 1;
		initWin();
      }
      else if (boardArr[1][3] == 79)
      {
        display_string(0, "O WINS!");
        display_update();
        win = 2;
		initWin();
      }
	}
	else if(count == 9 && win == 0)
  	{
		display_string(0, "It's a TIE!");
		display_update();
		win = 3;
    	delay(250);
		resetGame();
		delay(100);
		menu();
		display_update();
  	}

}


void displayTurn(void)
{
  if(winExists() == 0){
    if(turn == 1)
    {
      textbuffer[2][8] = 88; // X 
    }
    else if (turn == 2)
    {
      textbuffer[2][8] = 79; // O
    }

    textbuffer[2][10] = 84; // T
    textbuffer[2][11] = 85; // U
    textbuffer[2][12] = 82; // R
    textbuffer[2][13] = 78; // N
  }
}

void saveTemp(int Y, int X)
{
  temp = textbuffer[Y][X];
  display_update();
}

void writeTemp(int Y, int X){
  textbuffer[Y][X] = temp;
  display_update();
}

void createCursor(void) {
  checkWin();
  if(winExists() == 0)
  {
    displayTurn();
    timerStart = 1;
    temp = textbuffer[1][1];
    delay(250);
    textbuffer[1][1] = 43; //row 0 textbuffer unused
    currX = 1;
    currY = 1;
    count++;
  }
  display_update();
}

 //1 - left
 //2 - right
void moveCursor(int direction){
  if(direction == 1){
    if(currY != 1){
      if(currX == 1){
        if (currY == 2 || currY == 3){
        //Goes to prev line
        delay(250);
        writeTemp(currY,currX);
        currY--;
        currX = 5;
        saveTemp(currY,currX);
        }
      }
      else{
        writeTemp(currY,currX);
        delay(250);
        currX-=2;
        saveTemp(currY,currX);
      }
    }else{
      if(currX != 1){
       writeTemp(currY,currX);
        delay(250);
        currX-=2;
        saveTemp(currY,currX);
      }
      else{
        return;
      }
    }
  }
  else if(direction == 2){
    if(currY != 3){
      if(currX == 5){
        if (currY == 1 || currY == 2){
        writeTemp(currY,currX);
        delay(250);
        currY++;
        currX = 1;
        saveTemp(currY,currX);
        }
      }
      else{
       writeTemp(currY,currX);
        delay(250);
        currX+=2;
        saveTemp(currY,currX);
      }
    }else{
      if(currX != 5){
        writeTemp(currY,currX);
        delay(250);
        currX+=2;
        saveTemp(currY,currX);
      }
      else{
        return;
      }
    }
  }

  textbuffer[currY][currX] = 43;
  display_update();
  return;
}

void placeMarker(int type){
  if(type == 1){
    textbuffer[currY][currX] = 88; // X Turn
    boardArr[currY-1][currX] = 88;
  }
  else if(type == 2) {
    textbuffer[currY][currX] = 79; // O Turn
    boardArr[currY-1][currX] = 79;
  }
}

void place(void) {
  if(boardArr[currY-1][currX] == 43)
  {
    if (turn == 1)      //X
    {
      placeMarker(turn);
      turn = 2;
      display_update();
    }
    else if (turn == 2) //O
    {
      placeMarker(turn);
      turn = 1;
      display_update();
    }
	display_string(0, "  TicTacToe  "); //clr
    display_update();
    createCursor();
  }
  else
  {
    display_string(0, "ILLEGAL MOVE");
    display_update();
    return;
  }

}
