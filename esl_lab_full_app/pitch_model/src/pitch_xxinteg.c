/**********************************************************
 * This file is generated by 20-sim ANSI-C Code Generator
 *
 *  file:  pitch_xxinteg.c
 *  subm:  PositionControllerTilt
 *  model: PositionControllerTilt
 *  expmt: Jiwy-1
 *  date:  June 2, 2025
 *  time:  10:23:18 AM
 *  user:  Vakgroep RaM
 *  from:  -
 *  build: 5.1.4.13773
 **********************************************************/

/* This file describes the integration methods
   that are supplied for computation.

   Currently the following methods are supported:
   * Discrete
   * Euler
   * RungeKutta2
   * RungeKutta4
   but it is easy for the user to add their own
   integration methods with these as examples.
*/

/* the system include files */
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* our own include files */
#include "pitch_xxinteg.h"
#include "pitch_xxmodel.h"

/* global variables prototypes */
extern PITCH_XXDouble pitch_xx_time;
extern PITCH_XXDouble pitch_xx_step_size;

#define pitch_xx_STATE_SIZE 3

/*********************************************************************
 * Discrete integration method
 *********************************************************************/

/* the initialization of the Discrete integration method */
void PITCH_XXDiscreteInitialize (void)
{
	/* nothing to be done */
	pitch_xx_major = PITCH_XXTRUE;
}

/* the termination of the Discrete integration method */
void PITCH_XXDiscreteTerminate (void)
{
	/* nothing to be done */
}

/* the Discrete integration method itself */
void PITCH_XXDiscreteStep (void)
{
	PITCH_XXInteger index;

	/* for each of the supplied states */
	for (index = 0; index < pitch_xx_STATE_SIZE; index++)
	{
		/* just a move of the new state */
		pitch_xx_s [index] = pitch_xx_R [index];
	}
	/* increment the simulation time */
	pitch_xx_time += pitch_xx_step_size;

	pitch_xx_major = PITCH_XXTRUE;

	/* evaluate the dynamic part to calculate the new rates */
	PITCH_XXCalculateDynamic ();
}

