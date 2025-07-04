/**********************************************************
 * This file is generated by 20-sim ANSI-C Code Generator
 *
 *  file:  pitch_xxmodel.c
 *  model: PositionControllerTilt
 *  expmt: Jiwy-1
 *  date:  June 2, 2025
 *  time:  10:23:18 AM
 *  user:  Vakgroep RaM
 *  from:  -
 *  build: 5.1.4.13773
 **********************************************************/

/* This file contains the actual model variables and equations */

/* Note: Alias variables are the result of full optimization
   of the model in 20-sim. As a result, only the real variables
   are used in the model for speed. The user may also include
   the alias variables by adding them to the end of the array:

   PITCH_XXDouble pitch_xx_variables[NUMBER_VARIABLES + NUMBER_ALIAS_VARIABLES + 1];
   PITCH_XXString pitch_xx_variable_names[] = {
     VARIABLE_NAMES, ALIAS_VARIABLE_NAMES, NULL
   };

   and calculate them directly after the output equations:

   void PITCH_XXCalculateOutput (void)
   {
     OUTPUT_EQUATIONS
     ALIAS_EQUATIONS
   }
*/

/* system include files */
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* 20-sim include files */
#include "pitch_xxmodel.h"
#include "pitch_xxfuncs.h"

/* the global variables */
PITCH_XXDouble pitch_xx_start_time = 0.0;
PITCH_XXDouble pitch_xx_finish_time = 20.0;
PITCH_XXDouble pitch_xx_step_size = 0.01;
PITCH_XXDouble pitch_xx_time = 0.0;
PITCH_XXInteger pitch_xx_steps = 0;
PITCH_XXBoolean pitch_xx_initialize = PITCH_XXTRUE;
PITCH_XXBoolean pitch_xx_major = PITCH_XXTRUE;
PITCH_XXBoolean pitch_xx_stop_simulation = PITCH_XXFALSE;

/* the variable arrays */
PITCH_XXDouble pitch_xx_P[pitch_xx_parameters_size];		/* parameters */
PITCH_XXDouble pitch_xx_I[pitch_xx_initialvalues_size];		/* initial values */
PITCH_XXDouble pitch_xx_V[pitch_xx_variables_size];		/* variables */
PITCH_XXDouble pitch_xx_s[pitch_xx_states_size];		/* states */
PITCH_XXDouble pitch_xx_R[pitch_xx_states_size];		/* rates (or new states) */

/* the names of the variables as used in the arrays above
   uncomment this part if these names are needed
PITCH_XXString pitch_xx_parameter_names[] = {
	"corrGain\\K",
	"PID1\\kp",
	"PID1\\tauD",
	"PID1\\beta",
	"PID1\\tauI",
	"SignalLimiter2\\minimum",
	"SignalLimiter2\\maximum"
,	NULL
};
PITCH_XXString pitch_xx_initial_value_names[] = {
	"PID1\\uD_previous_initial",
	"PID1\\error_previous_initial",
	"PID1\\uI_previous_initial"
,	NULL
};
PITCH_XXString pitch_xx_variable_names[] = {
	"corrGain\\input",
	"corrGain\\output",
	"PID1\\output",
	"",
	"PlusMinus1\\output",
	"PlusMinus2\\plus1",
	"PlusMinus2\\minus1",
	"SignalLimiter2\\output",
	"corr",
	"in",
	"position",
	"out"
,	NULL
};
PITCH_XXString pitch_xx_state_names[] = {
	"PID1\\uD_previous",
	"PID1\\error_previous",
	"PID1\\uI_previous"
,	NULL
};
PITCH_XXString pitch_xx_rate_names[] = {
	"",
	"PID1\\error",
	""
,	NULL
};
*/

#if (7 > 8192) && defined _MSC_VER
#pragma optimize("", off)
#endif
void PITCH_XXModelInitialize_parameters(void)
{
	/* set the parameters */
	pitch_xx_P[0] = 0.0;		/* corrGain\K */
	pitch_xx_P[1] = 1.6;		/* PID1\kp */
	pitch_xx_P[2] = 0.05;		/* PID1\tauD */
	pitch_xx_P[3] = 0.001;		/* PID1\beta */
	pitch_xx_P[4] = 10.5;		/* PID1\tauI */
	pitch_xx_P[5] = -0.99;		/* SignalLimiter2\minimum */
	pitch_xx_P[6] = 0.99;		/* SignalLimiter2\maximum */

}
#if (7 > 8192) && defined _MSC_VER
#pragma optimize("", on)
#endif

void PITCH_XXModelInitialize_initialvalues(void)
{
	/* set the initial values */
	pitch_xx_I[0] = 0.0;		/* PID1\uD_previous_initial */
	pitch_xx_I[1] = 0.0;		/* PID1\error_previous_initial */
	pitch_xx_I[2] = 0.0;		/* PID1\uI_previous_initial */

}

void PITCH_XXModelInitialize_states(void)
{
	/* set the states */
	pitch_xx_s[0] = pitch_xx_I[0];		/* PID1\uD_previous */
	pitch_xx_s[1] = pitch_xx_I[1];		/* PID1\error_previous */
	pitch_xx_s[2] = pitch_xx_I[2];		/* PID1\uI_previous */

}

void PITCH_XXModelInitialize_variables(void)
{
	/* initialize the variable memory to zero */
	memset(pitch_xx_V, 0, pitch_xx_variables_size * sizeof(PITCH_XXDouble));
}

/* this method is called before calculation is possible */
void PITCH_XXModelInitialize (void)
{
	PITCH_XXModelInitialize_parameters();
	PITCH_XXModelInitialize_variables();
	PITCH_XXModelInitialize_initialvalues();
	PITCH_XXModelInitialize_states();
}

/* This function calculates the initial equations of the model.
 * These equations are calculated before anything else
 */
void PITCH_XXCalculateInitial (void)
{

	/* set the states again, they might have changed in the initial calculation */
	PITCH_XXModelInitialize_states ();
}

/* This function calculates the static equations of the model.
 * These equations are only dependent from parameters and constants
 */
void PITCH_XXCalculateStatic (void)
{

}

/* This function calculates the input equations of the model.
 * These equations are dynamic equations that must not change
 * in calls from the integration method (like random and delay).
 */
void PITCH_XXCalculateInput (void)
{

}

/* This function calculates the dynamic equations of the model.
 * These equations are called from the integration method
 * to calculate the new model rates (that are then integrated).
 */
void PITCH_XXCalculateDynamic (void)
{
	/* PID1\factor = 1 / (sampletime + PID1\tauD * PID1\beta); */
	pitch_xx_V[3] = 1.0 / (pitch_xx_step_size + pitch_xx_P[2] * pitch_xx_P[3]);

	/* corrGain\input = corr; */
	pitch_xx_V[0] = pitch_xx_V[8];

	/* PlusMinus2\plus1 = in; */
	pitch_xx_V[5] = pitch_xx_V[9];

	/* PlusMinus2\minus1 = position; */
	pitch_xx_V[6] = pitch_xx_V[10];

	/* corrGain\output = corrGain\K * corrGain\input; */
	pitch_xx_V[1] = pitch_xx_P[0] * pitch_xx_V[0];

	/* PID1\error = PlusMinus2\plus1 - PlusMinus2\minus1; */
	pitch_xx_R[1] = pitch_xx_V[5] - pitch_xx_V[6];

	/* PID1\uD = PID1\factor * (((PID1\tauD * PID1\uD_previous) * PID1\beta + (PID1\tauD * PID1\kp) * (PID1\error - PID1\error_previous)) + (sampletime * PID1\kp) * PID1\error); */
	pitch_xx_R[0] = pitch_xx_V[3] * (((pitch_xx_P[2] * pitch_xx_s[0]) * pitch_xx_P[3] + (pitch_xx_P[2] * pitch_xx_P[1]) * (pitch_xx_R[1] - pitch_xx_s[1])) + (pitch_xx_step_size * pitch_xx_P[1]) * pitch_xx_R[1]);

	/* PID1\uI = PID1\uI_previous + (sampletime * PID1\uD) / PID1\tauI; */
	pitch_xx_R[2] = pitch_xx_s[2] + (pitch_xx_step_size * pitch_xx_R[0]) / pitch_xx_P[4];

	/* PID1\output = PID1\uI + PID1\uD; */
	pitch_xx_V[2] = pitch_xx_R[2] + pitch_xx_R[0];

	/* PlusMinus1\output = corrGain\output + PID1\output; */
	pitch_xx_V[4] = pitch_xx_V[1] + pitch_xx_V[2];

	/* SignalLimiter2\output = if PlusMinus1\output < SignalLimiter2\minimum... ; */
	pitch_xx_V[7] = ((pitch_xx_V[4] < pitch_xx_P[5]) ? 
		/* SignalLimiter2\minimum */
		pitch_xx_P[5]
	:
		/* if PlusMinus1\output > SignalLimiter2\maximum...  */
		((pitch_xx_V[4] > pitch_xx_P[6]) ? 
			/* SignalLimiter2\maximum */
			pitch_xx_P[6]
		:
			/* PlusMinus1\output */
			pitch_xx_V[4]
		)
	);


	/* increment the step counter */
	pitch_xx_steps++;
}

/* This function calculates the output equations of the model.
 * These equations are not needed for calculation of the rates
 * and are kept separate to make the dynamic set of equations smaller.
 * These dynamic equations are called often more than one time for each
 * integration step that is taken. This makes model computation much faster.
 */
void PITCH_XXCalculateOutput (void)
{
	/* out = SignalLimiter2\output; */
	pitch_xx_V[11] = pitch_xx_V[7];

}

/* This function calculates the final equations of the model.
 * These equations are calculated after all the calculations
 * are performed
 */
void PITCH_XXCalculateFinal (void)
{

}

/* this method is called after all calculations are performed */
void PITCH_XXModelTerminate(void)
{
}

