/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 *
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

// Main program for exercise

//****************************************************
//By default, every output used in this exercise is 0
//****************************************************
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xgpiops.h"
#include "xttcps.h"
#include "xscugic.h"
#include "xparameters.h"
#include "Pixel.h"
#include "Interrupt_setup.h"

//********************************************************************
//***************TRY TO READ COMMENTS*********************************
//********************************************************************

//***Hint: Use sleep(x)  or usleep(x) if you want some delays.****
//***To call assembler code found in blinker.S, call it using: blinker();***


//Comment this if you want to disable all interrupts
#define enable_interrupts

void DrawShip();
void ResetGame();
void DeleteShip();
void MoveBullet();
void MoveAlien();
void UpdateScore();
void RespawnAlien();
void PrintWin();
void PrintLoss();

/***************************************************************************************
Name:
Aaron Hirvi
050320442

Name:
Ilari Miettunen
050371213

Tick boxes that you have coded

Led-matrix driver		Game		    Assembler
	[x]					[x]					[]

Brief description:
Pelissä käyttäjä ampuu sivuille ohjattavalla aluksella alaspäin liikkuvaa alienia.
Pelaajan osuessa alienia, pelaaja saa pisteen. Alienin osuessa alukseen, alien saa pisteen.
Pisteet näkyvät oikeassa reunassa.
Ensin neljä pistettä saanut voittaa.

BTN3: Ohjaa vasemmalle
BTN2: Ohjaa oikealle
BTN1: Ammu aluksella
BTN0: Aloita peli alusta

SW1: Vaihtaa pelin päivitystaajuutta 10Hz <-> 20Hz
SW0: Vaihtaa pelin vaikeustasoa eli alienin nopeutta
*****************************************************************************************/

volatile uint32_t channel = 0;

// Aluksen koordinaatit
volatile uint8_t x = 3;
volatile uint8_t y = 7;

// Aluksen väri
volatile uint8_t r = 255;
volatile uint8_t b = 255;
volatile uint8_t g = 255;


// Alienin koordinaatit
volatile uint8_t alien_x = 0;
volatile uint8_t alien_y = 0;

// Alienin suunta
volatile int is_going_left = 0;

// Alienin väri
volatile uint8_t alien_r = 0;
volatile uint8_t alien_g = 255;
volatile uint8_t alien_b = 0;


// Timer jolla lasketaan alienin liike
volatile uint32_t timer = 0;
volatile int timer_state = 1;


// Pelin vaikeus
volatile uint32_t difficulty = 2;
volatile uint32_t freq = 5;


// Ammuksen koordinaatit
volatile uint8_t bullet_x = 0;
volatile uint8_t bullet_y = 0;

// Ammuksen totuusarvo
volatile int bullet_exist = 0;

// Ammuksen väri
volatile uint8_t bullet_r = 100;
volatile uint8_t bullet_g = 100;
volatile uint8_t bullet_b = 100;


// Pelin pisteet
volatile uint8_t player_score = 0;
volatile uint8_t score_xpos = 7;
volatile uint8_t alien_score = 0;



int main()
{
	//**DO NOT REMOVE THIS****
	    init_platform();
	//************************


#ifdef	enable_interrupts
	    init_interrupts();


#endif


	    //setup screen
	    setup();

	    // first render of alien
	    SetPixel(alien_x,alien_y,alien_r,alien_g,alien_b);

	    Xil_ExceptionEnable();



	    //Try to avoid writing any code in the main loop.
		while(1){


		}


		cleanup_platform();
		return 0;
}


//Timer interrupt handler for led matrix update. Frequency is 800 Hz
void TickHandler(void *CallBackRef){
	//Don't remove this
	uint32_t StatusEvent;

	//exceptions must be disabled when updating screen
	Xil_ExceptionDisable();


	//****Write code here ****
	//- To update the matrix, it should be first checked that the channel is not greater than 7.
	if (channel>7)  channel=0;

	//- Next, all channels should be closed with the default case of open_line().
	open_line(8);

	//- After this, the current channel should be run and opened, and the channel should be incremented.
	run(channel);
	open_line(channel);
	channel++;


	//****END OF OWN CODE*****************

	//*********clear timer interrupt status. DO NOT REMOVE********
	StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
	XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);
	//*************************************************************
	//enable exceptions
	Xil_ExceptionEnable();
}


//Timer interrupt for moving alien, shooting... Frequency is 10 Hz by default
void TickHandler1(void *CallBackRef){

	//Don't remove this
	uint32_t StatusEvent;

	// Päivittää aluksen
	DrawShip();

	// Ammuksen ja alienin liikutus
	// Liikuttaa ammusta nopeammin kuin alienia vaikeustasosta riippuen
	if(timer_state == 1){
		if(bullet_exist == 1){
			MoveBullet();
		}

		timer++;
		if(timer >= difficulty){
			MoveAlien();
			timer = 0;
		}
	}


	// Ammus ja alien törmäävät
	if(alien_x == bullet_x && alien_y == bullet_y && alien_x != 0 && alien_y !=0){
		SetPixel(bullet_x,bullet_y,0,0,0);
		player_score += 1;
		SetPixel(score_xpos, 4-player_score, 0,255,0);

		RespawnAlien();
	}

	// Alien ja alus törmäävät
	if(x == alien_x && y == alien_y+1){
		alien_x = 0;
		alien_y = 0;

		SetPixel(bullet_x,bullet_y,0,0,0);

		alien_score += 1;
		SetPixel(score_xpos, 3+alien_score, 255,0,0);

		RespawnAlien();
	}

	// Pelin voittaimsen tai häviämisen tarkastelu
	if(player_score == 4 || alien_score == 4){
		timer_state = 0;

		if(player_score == 4){
			PrintWin();
		}else{
			PrintLoss();
		}


	}

	//clear timer interrupt status. DO NOT REMOVE
	StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
	XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);

}



//Interrupt handler for switches and buttons.
//Reading Status will tell which button or switch was used
//Bank information is useless in this exercise
void ButtonHandler(void *CallBackRef, u32 Bank, u32 Status){


	// reset game
	if(Status==0x01){
		ResetGame();
	}

	//BTN1 is clicked
	// Ammu ammus
	else if(Status==0x02){
		// Voi ampua vain jos alien on aktiivinen ja ammuksia ei ole jo olemassa
		if(timer_state==1 && bullet_exist == 0){
			bullet_x = x;
			bullet_y = y-2;
			bullet_exist = 1;

			// Alustetaan ammus
			SetPixel(bullet_x,bullet_y,bullet_r,bullet_g,bullet_b);
		}

	}
	//BTN2 is clicked
	else if(Status==0x04){
		if(x < 6){
			// Liikuta alusta
			DeleteShip();
			x += 1;
		}
	}
	//BTN3 is clicked
	else if(Status==0x08){
		if(x > 0){
			// Liikuta alusta
			DeleteShip();
			x -= 1;
		}
	}
	//SW0 position is changed. 0xE000A068 address needs to be read if want to know in which position slider is
	else if(Status==0x10){
		// Vaihdetaan vaikeustasoa (alienin nopeutta)
		if(difficulty == 2){
			difficulty = 4;
		}else{
			difficulty = 2;
		}
	}
	//SW1 position is changed. 0xE000A068 address needs to be read if want to know in which position slider is
	else if(Status==0x20){
		// Vaihdetaan päivitysnopeutta
		if(freq == 20){
			freq = 10;
			change_freq(freq);
		}else{
			freq = 20;
			change_freq(freq);
	        }
	    }

}


