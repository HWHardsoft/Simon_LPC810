/**
* @file main.c
*
* @brief electronic game Simon says (aka Senso) with LPC810 (ARM Cortex M0+)
*
* @version 1.1
*
* @date 2014-03-31
*
* @author  Hartmut Wendt  info@hwhardsoft.de	<http://www.hwhardsoft.de>
*
* @copyright GNU Public License V3
*
* @note
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
//#define _debug

#include <stdio.h>
#include "LPC8xx.h"
#include "gpio.h"
#include "mrt.h"
#include "sct.h"
#include "sct.h"
#include "lpc8xx_pmu.h"
#if defined(_debug)
 #include "uart.h"
#endif

#if defined(__CODE_RED)
  #include <cr_section_macros.h>
  #include <NXP/crp.h>
  __CRP const unsigned int CRP_WORD = CRP_NO_CRP ;
#endif


#define LED_GREEN	4
#define LED_RED		0
#define LED_YELLOW	3
#define LED_BLUE	2

#define SND_GREEN	(12000)
#define SND_RED		(10000)
#define SND_BLUE	(6000)
#define SND_YELLOW	(8000)


// general parameters
#define max_steps	100
#define max_timeout	250		// 5s

  // Program modes
  enum {
  	PM_Simon_says,
  	PM_Player_says,
  	PM_Game_start,
  };

  volatile unsigned char steps[max_steps + 1];
  volatile unsigned char current_step=0;
  volatile unsigned char actual_step=0;
  volatile unsigned char program_mode = PM_Game_start;
  volatile unsigned char Random = 0;
  volatile unsigned int button_timeout_cnt=0;

/********************************************************************//**
 * @brief 		configure the pins for normal operation
 * @param[in]		none
 * @return		none
 *********************************************************************/
void configurePins()
{
	   /* Enable SWM clock */
	    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7);

	    /* Pin Assign 8 bit Configuration */
	    /* CTOUT_0 */
	    LPC_SWM->PINASSIGN6 = 0x01ffffffUL;
	    //LPC_SWM->PINASSIGN6 = 0x00ffffffUL;

	    /* Pin setup generated via Switch Matrix Tool
	        ------------------------------------------------
	        PIO0_5 = GPIO
	        PIO0_4 = GPIO
	        PIO0_3 = GPIO            - Disables SWDCLK
	        PIO0_2 = GPIO 			 - Disables SWDIO
	        PIO0_1 = CTOUT_0
	        PIO0_0 = GPIO
	        ------------------------------------------------
	        NOTE: SWD is disabled to free GPIO pins!
	        ------------------------------------------------ */
	    /* Pin Assign 1 bit Configuration */
	    LPC_SWM->PINENABLE0 = 0xffffffbfUL;
}

/********************************************************************//**
 * @brief 		configure the pins for debug mode
 * @param[in]	none
 * @return		none
 *********************************************************************/
void configurePins_debug()
{
/* Enable SWM clock */
 LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7);

 /* Pin Assign 8 bit Configuration */
 /* U0_TXD */
 LPC_SWM->PINASSIGN0 = 0xffffff04UL;

 /* Pin Assign 1 bit Configuration */
 //LPC_SWM->PINENABLE0 = 0xffffffbfUL;
 LPC_SWM->PINENABLE0 = 0xffffffffUL;

}


/********************************************************************//**
 * @brief 		set port as output and to low level for LED on
 * @param[in]	ucled (port pin)
 * @return		none
 *********************************************************************/
void set_led(unsigned char ucled)
{
  /* Set the LED pin to output (1 = output, 0 = input) */
  gpioSetDir(0,	ucled, 1);
  //LPC_GPIO_PORT->DIR0 |= (1 << ucled);
  /* Turn LED On by setting the GPIO pin low */
  gpioSetValue(0, ucled, 0);
  //LPC_GPIO_PORT->CLR0 = 1 << ucled;
}

/********************************************************************//**
 * @brief 		set port as input for button detection
 * @param[in]	ucled (port pin)
 * @return		none
 *********************************************************************/
void reset_led(unsigned char ucled)
{
  /* Set the LED pin to input (1 = output, 0 = input) */
  gpioSetDir(0,	ucled, 0);
  //LPC_GPIO_PORT->DIR0 &= ~(1 << ucled);
}


/********************************************************************//**
 * @brief 		get the status (pressed / unpressed) of a button
 * @param[in]	ucled (port pin)
 * @return		status (0 = unpressed, 1 = pressed)
 *********************************************************************/
char get_button_status(unsigned char ucled) {
	if (gpioGetPinValue(0,ucled)) return(0);
	else return(1);
}


/********************************************************************//**
 * @brief 		starts beeper according LED colour
 * @param[in]	ucled (led colour)
 * @return		none
 *********************************************************************/
void start_sound(unsigned char ucled) {
	if (ucled == LED_RED) beep_start(SND_RED);
	else if (ucled == LED_BLUE) beep_start(SND_BLUE);
	else if (ucled == LED_GREEN) beep_start(SND_GREEN);
	else beep_start(SND_YELLOW);
}

/********************************************************************//**
 * @brief 		ends user input after wrong input
 * @param[in]	none
 * @return		none
 *********************************************************************/
void simon_end(void)
{
  #if defined(_debug)
	printf("wrong key\n");
  #endif
  // blink for end
  set_led(LED_RED);
  set_led(LED_BLUE);
  set_led(LED_GREEN);
  set_led(LED_YELLOW);
  // stop sound
  beep_stop();
  mrtDelay(500);
  reset_led(LED_RED);
  reset_led(LED_BLUE);
  reset_led(LED_GREEN);
  reset_led(LED_YELLOW);
  //show tip
  mrtDelay(200);
  set_led(steps[actual_step]);
  start_sound(steps[actual_step]);
  mrtDelay(500);
  reset_led(steps[actual_step]);
  beep_stop();

  // goto deep power down mode
  PMU_DeepPowerDown();
  program_mode = PM_Game_start;
}


int main(void)
{
  unsigned char b;

  /* Configure the core clock/PLL via CMSIS */
  SystemCoreClockUpdate();

  /* Initialise the GPIO block */
  gpioInit();
  /* set direction to input */
  gpioSetDir(0,LED_BLUE,0);
  gpioSetDir(0,LED_RED,0);
  gpioSetDir(0,LED_GREEN,0);
  gpioSetDir(0,LED_YELLOW,0);


#if defined(_debug)
  /* Initialise the UART0 block for printf output */
  uart0Init(115200);
#endif

  /* Configure the multi-rate timer for 1ms ticks */
  mrtInit(SystemCoreClock/1000);

#if defined(_debug)
  /* Configure the switch matrix (setup pins for UART and GPIO) */
  configurePins_debug();
#else
  /* Configure the switch matrix (setup pins for SCT and GPIO) */
  configurePins();
#endif

  // enable the SCT clock
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 8);

  // clear peripheral reset the SCT:
  LPC_SYSCON->PRESETCTRL &= ~(1 << 8);
  LPC_SYSCON->PRESETCTRL |= (1 << 8);

  /* Initialise the SCT timer */
  sct_init();
  // unhalt it: - clearing bit 2 of the CTRL register
  LPC_SCT->CTRL_L &= ~( 1<< 2  );

  beep_stop();

#if defined(_debug)
  printf("Program start\n");
#endif



  while(1)
  {

	  switch(program_mode)
	  {
	  	  // game start
  	  	  case PM_Game_start:
  	  		  // clear Deep Power Down Flag in PCON register
 	  		  LPC_PMU->PCON |= (1 << 11);

  	  		  // wait for release of all buttons
  	  		  while (get_button_status(LED_RED) || get_button_status(LED_GREEN) ||
  	  		  		 get_button_status(LED_YELLOW) || get_button_status(LED_BLUE)) Random++;

  	  		  // blink all LEDs
  	  		  set_led(LED_RED);
  	  		  set_led(LED_BLUE);
  	  		  set_led(LED_GREEN);
  	  		  set_led(LED_YELLOW);

  	  		  mrtDelay(200);
  	  		  reset_led(LED_RED);
  	  		  reset_led(LED_BLUE);
  	  		  reset_led(LED_GREEN);
  	  		  reset_led(LED_YELLOW);
  	  		  mrtDelay(500);
  	  		  program_mode = PM_Simon_says;

  	  		  // reset game variables
  	  	      current_step=0;
  	  	      actual_step=0;
  	  	      button_timeout_cnt = 0;
  	  		  break;


	  	  // simon says	..
	  	  case PM_Simon_says:

	  		  // calculate next step and increase the count of steps
			#if defined(_debug)
	  		  printf("Mode Simon says\n");
			#endif

	  		  Random = Random % 4;
	  		  if (Random == 1) steps[current_step++] = LED_RED;
	  		  else if (Random == 2) steps[current_step++] = LED_BLUE;
	  		  else if (Random == 3) steps[current_step++] = LED_GREEN;
	  		  else steps[current_step++] = LED_YELLOW;

	  		  // check maximal possible count of steps
	  		  if (current_step >= max_steps) {
	  			  simon_end();
	  			  break;
	  		  }

	  		  mrtDelay(500);
	  		  for(b=0; b < current_step; b++) {
	  			  set_led(steps[b]);
	  			  // start beeper
	  			  start_sound(steps[b]);

	  			  mrtDelay(500);
	  			  reset_led(steps[b]);
	  			  beep_stop();	// stops beeper
	  			  mrtDelay(500);
	  		  }
	  		  // reset counter
	  		  actual_step=0;

	  		  // switch to player mode
	  		  program_mode = PM_Player_says;
	  		  button_timeout_cnt = 0;
	  		  break;

	  	  // player says...
	  	  case PM_Player_says:

	  		  if (actual_step < current_step) {

	  			  // button red pressed
	  			  if (get_button_status(LED_RED)) {
					#if defined(_debug)
	  				  printf("Button red pressed\n");
					#endif

	  				  beep_start(SND_RED);
	  				  if (LED_RED != steps[actual_step]) {
	  					  simon_end();
	  					  break;
	  				  }
	  				  actual_step++;
	  				  mrtDelay(10);
	  				  button_timeout_cnt = 0;

	  				  // wait for release of button red
	  				  while (get_button_status(LED_RED)) Random++;
	  				  //reset_led(LED_RED);
	  			  }

	  			  // button green pressed
	  			  else if (get_button_status(LED_GREEN)) {
					#if defined(_debug)
	  				  printf("Button green pressed\n");
					#endif
	  				  beep_start(SND_GREEN);
	  				  if (LED_GREEN != steps[actual_step]) {
	  					  simon_end();
	  					  break;
	  				  }
	  				  actual_step++;
	  				  mrtDelay(10);
	  				  button_timeout_cnt = 0;

	  				  // wait for release of button red
	  				  while (get_button_status(LED_GREEN)) Random++;
	  				  //reset_led(LED_GREEN);
	  			   }

	  			   // button blue pressed
	  			   else if (get_button_status(LED_BLUE)) {
				 	 #if defined(_debug)
		  			   printf("Button blue pressed\n");
					 #endif
		  			 beep_start(SND_BLUE);
		  			   if (LED_BLUE != steps[actual_step]) {
		  				  simon_end();
		  				  break;
		  				}
		  			    actual_step++;
		  				mrtDelay(10);
		  				button_timeout_cnt = 0;

		  				// wait for release of button blue
		  				while (get_button_status(LED_BLUE)) Random++;
		  				//reset_led(LED_BLUE);
		  			 }

		  			 // button yellow pressed
	  			     else if (get_button_status(LED_YELLOW)) {
						#if defined(_debug)
		  				  printf("Button yellow pressed\n");
						#endif
		  				  //set_led(LED_YELLOW);
		  				  beep_start(SND_YELLOW);

		  				  if (LED_YELLOW != steps[actual_step]) {
		  					  simon_end();
		  					  break;
		  				  }
		  				  actual_step++;
		  				  mrtDelay(10);
		  				  button_timeout_cnt = 0;

		  				  // wait for release of button yellow
		  				  while (get_button_status(LED_YELLOW)) Random++;
		  				  //reset_led(LED_YELLOW);

		  			  }

	  			  	  // no button pressed
	  			      else {
	  			    	  button_timeout_cnt++;
	  			    	  if (button_timeout_cnt >= max_timeout) {
	  			    		  simon_end();
	  			    		  break;
	  			    	  }

	  			      }
		  			  beep_stop();
		  			  mrtDelay(20);

	  		  } else {
	  			  program_mode = PM_Simon_says;
	  		  }

	  		  break;
	  }
  }
}
