/**
* @file sct.c
*
* @brief sct timer with match compared output for beeper
*
* @version 1.0
*
* @date 2013-06-27
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
 #include "sct.h"

/********************************************************************//**
 * @brief 		init the sct timer as match output at CT_OUT0
 * @param[in]	none
 * @return		none
 *********************************************************************/
void sct_init (void)
{
	LPC_SCT->CONFIG = (LPC_SCT->CONFIG & ~0x00060001) | 0x00000001; /* UNIFIED */

	/* MATCH/CAPTURE registers */

	/* Unified counter - register side L is used and accessed as 32 bit value, reg H is not used */
	LPC_SCT->REGMODE_L = 0x00000000;         /* U: 1x MATCH, 0x CAPTURE, 4 unused */

	LPC_SCT->MATCH[0].U = 10000;             /* matchOnDelay */
	LPC_SCT->MATCHREL[0].U = 10000;

	/* OUTPUT registers */
	LPC_SCT->OUT[0].SET = 0x00000002;        /* Output_pin_0 */
	LPC_SCT->OUT[0].CLR = 0x00000001;
	  /* Unused outputs must not be affected by any event */
	LPC_SCT->OUT[1].SET = 0;
	LPC_SCT->OUT[1].CLR = 0;
	LPC_SCT->OUT[2].SET = 0;
	LPC_SCT->OUT[2].CLR = 0;
	LPC_SCT->OUT[3].SET = 0;
	LPC_SCT->OUT[3].CLR = 0;
	LPC_SCT->OUTPUT = (LPC_SCT->OUTPUT & ~0x00000000) | 0x00000001;

	/* Conflict resolution register */

	/* EVENT registers */
	LPC_SCT->EVENT[0].CTRL = 0x0000D000;     /* U: --> state state_1 */
	LPC_SCT->EVENT[0].STATE = 0x00000001;
	LPC_SCT->EVENT[1].CTRL = 0x00005000;     /* U: --> state U_ENTRY */
	LPC_SCT->EVENT[1].STATE = 0x00000002;
	  /* Unused events must not have any effect */
	LPC_SCT->EVENT[2].STATE = 0;
	LPC_SCT->EVENT[3].STATE = 0;
	LPC_SCT->EVENT[4].STATE = 0;
	LPC_SCT->EVENT[5].STATE = 0;

	/* STATE registers */
	LPC_SCT->STATE_L = 0;

	/* state names assignment: */
	  /* State U 0: U_ENTRY */
	  /* State U 1: state_1 */

	/* CORE registers */
	LPC_SCT->START_L = 0x00000000;
	LPC_SCT->STOP_L =  0x00000000;
	LPC_SCT->HALT_L =  0x00000000;
	LPC_SCT->LIMIT_L = 0x00000003;
	LPC_SCT->EVEN =    0x00000000;


}


/********************************************************************//**
 * @brief 		stop the sct timer / beeper
 * @param[in]	none
 * @return		none
 *********************************************************************/
void beep_stop(void)
{
	// Set Halt and Stop.
	LPC_SCT->CTRL_U |= 0x00000006;
}


/********************************************************************//**
 * @brief 		start the sct timer / beeper
 * @param[in]	frequency
 * @return		none
 *********************************************************************/
void beep_start(uint32_t value)
{
	// Set CLRCTR.
	 LPC_SCT->CTRL_U |= (1<<3);

	LPC_SCT->MATCH[0].U = value;             // matchOnDelay
	LPC_SCT->MATCHREL[0].U = value;

	// Clear Halt and Stop.
	LPC_SCT->CTRL_U &= 0xfffffff9;
}

