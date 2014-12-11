/********************************************************************
 * COPYRIGHT -- Microsoft
 ********************************************************************
 * Program: 5Vpulse
 * File: 5Vpulse.c
 * Author: palmerk
 * Created: August 14, 2014
 ********************************************************************
 * Implementation of program 5Vpulse
 ********************************************************************/

#include <bur/plctypes.h>
#include "loopcont.h"

#ifdef _DEFAULT_INCLUDES
	#include <AsDefault.h>
#endif

LCPWM_typ LCPWM_0;
LCCounter_typ baseLCCounter;
_GLOBAL BOOL pulse;
REAL angle;

void _INIT VpulseInit(void)
{
	LCPWM_0.enable      = 1;
	LCPWM_0.max_value   = 2;
	LCPWM_0.min_value   = -2;
	LCPWM_0.t_min_pulse = 8;
	LCPWM_0.t_period    = 16;
}

void _CYCLIC VpulseCyclic(void)
{
	LCCounter(&baseLCCounter);
	
	angle += 0.001;
    
	/* Pulse width modulator */
	LCPWM_0.x        = (INT)(sinf(angle));
	//LCPWM_0.x        = (INT)(10000 * (1 + sinf(angle)));
	LCPWM_0.basetime = baseLCCounter.mscnt;
	LCPWM(&LCPWM_0);		/* LCPWM function block call */
	pulse            = LCPWM_0.pulse;

	
/*
	angle += 0.008;
    
	// Pulse width modulator 
	LCPWM_0.x        = (INT)(0.25 * (0.25 + cosf(2*3.14* 5* angle)));
	LCPWM_0.basetime = baseLCCounter.mscnt;
	LCPWM(&LCPWM_0);		// LCPWM function block call 
	pulse = LCPWM_0.pulse;
*/
	
}
