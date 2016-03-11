/*
  Coded by Steven Moran and Cheuk Yu, with reference to MATLAB code: https://github.com/smoran1/matlab-scripts/tree/master/convolutional-encoding
*/

#include "interrupt.h"
#include "soc_OMAPL138.h"
#include "hw_syscfg0_OMAPL138.h"
#include "timer.h"
#include "lcdkOMAPL138.h"
#include "L138_LCDK_aic3106_init.h"
#include "L138_LCDK_switch_led.h"


/******************************************************************************
**                      INTERNAL VARIABLE DEFINITIONS
*******************************************************************************/

int LEN;
int b[1026];
int s[3072];
int y[3072];
int u[24] = { 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1, 1, -1, 1, -1, 1, -1 };
int A[16] = { 0, 1, -1, -1, -1, -1, 0, 1, 0, 1, -1, -1, -1, -1, 0, 1 };
int ip[8];
int nStates = 4; // Number of possible states
int d[4100];	//distance matrix
int p[4100];

int output[1024];

/******************************************************************************
**                          FUNCTION DEFINITIONS
*******************************************************************************/


int main(void)
{
	memset(s, 0, sizeof(s));
	memset(ip, 0, sizeof(ip));
	memset(d, 0, sizeof(d));
	mem_init();

	/*
		Summary:
		-ENCODER: 'message' (LEN) --> Convolutional Encoder --> 's' (3*LEN)
		-DECODER: 's' (3*LEN) --> Viterbi Decoder --> 'output' (LEN)
	*/

	//Create message to transmit
	LEN = 1024;

	b[0] = 0;  // initial condition must equal 0
	b[1] = 0;  // initial condition must equal 0
	
	//Information bits start here...
	//start with '1111000111100101010...' pattern
	b[2] = 1;
	b[3] = 1;
	b[4] = 1;
	b[5] = 1;
	b[6] = 0;
	b[7] = 0;
	b[8] = 0;
	b[9] = 1;
	b[10] = 1;
	b[11] = 1;
	b[12] = 1;
	b[13] = 0;
	b[14] = 0;

	int x;
	for (x = 15; x < LEN + 2; x++){
		if (x % 2 == 0) // Create alternating pattern of 0's and 1's
			b[x] = 0;
		else
			b[x] = 1;
	}


	// Create Codeword
	// ===============
	// convolutional code implemented is represented
	// by the following set of equations:
	// s_3n = b_n
	// s_3n-1 = b_n + b_n-2
	// s_3n-2 = b_n + b_n-1 + b_n-2

	for (x = 0; x < 3 * LEN; x++){
		s[x] = 0; //initialize s array to 0
	}

	for (x = 0; x < 3 * LEN; x++){
		s[3 * x + 2] = b[x + 2];
		s[3 * x + 1] = b[x + 2] + b[x];
		if (s[3 * x + 1] == 2)	// binary addition --> set to zero
			s[3 * x + 1] = 0;
		s[3 * x] = b[x + 2] + b[x + 1] + b[x];
		if (s[3 * x] == 2)
			s[3 * x] = 0;
		else if (s[3 * x] == 3)
			s[3 * x] = 1;
	}

	// Received
	for (x = 0; x<3 * LEN; x++){
		if (s[x] == 0)
			y[x] = 1;
		else
			y[x] = -1;
	}


	// Viterbi decoder
	// ---------------

	d[0] = 0;
	d[1] = -2147483000; // -inf
	d[2] = -2147483000; // -inf
	d[3] = -2147483000; // -inf

	for (x = 4; x < LEN + 1; x++){
		d[x] = 0; // initialize all values to 0
	}

	for (x = 0; x < 8; x++){
		ip[x] = 0; // initialize all values to 0
	}

	// Initialize distances
	printf("Initializing distances...\n");
	int y_b[3];
	for (x = 1; x < LEN + 1; x++)
	{
		// Select set of received bits
		y_b[0] = y[3 * (x - 1)];
		y_b[1] = y[3 * (x - 1) + 1];
		y_b[2] = y[3 * (x - 1) + 2];

		//Find inner products of y with transition outputs
		ip[0] = y_b[0] * u[0] + y_b[1] * u[1] + y_b[2] * u[2] + d[4 * (x - 1)];
		ip[1] = y_b[0] * u[3] + y_b[1] * u[4] + y_b[2] * u[5] + d[4 * (x - 1)];
		ip[2] = y_b[0] * u[6] + y_b[1] * u[7] + y_b[2] * u[8] + d[4 * (x - 1) + 1];
		ip[3] = y_b[0] * u[9] + y_b[1] * u[10] + y_b[2] * u[11] + d[4 * (x - 1) + 1];
		ip[4] = y_b[0] * u[12] + y_b[1] * u[13] + y_b[2] * u[14] + d[4 * (x - 1) + 2];
		ip[5] = y_b[0] * u[15] + y_b[1] * u[16] + y_b[2] * u[17] + d[4 * (x - 1) + 2];
		ip[6] = y_b[0] * u[18] + y_b[1] * u[19] + y_b[2] * u[20] + d[4 * (x - 1) + 3];
		ip[7] = y_b[0] * u[21] + y_b[1] * u[22] + y_b[2] * u[23] + d[4 * (x - 1) + 3];

		//d(1)
		if (ip[0] >= ip[4]){
			d[4 * (x)] = ip[0];
			p[4 * (x)] = 1;
		}
		else if (ip[0] < ip[4]){
			d[4 * (x)] = ip[4];
			p[4 * (x)] = 3;
		}
		//d(2)
		if (ip[1] >= ip[5]){
			d[4 * (x)+1] = ip[1];
			p[4 * (x)+1] = 1;
		}
		else if (ip[1] < ip[5]){
			d[4 * (x)+1] = ip[5];
			p[4 * (x)+1] = 3;
		}
		//d(3)
		if (ip[2] >= ip[6] && ip[2] >= -2147480000){
			d[4 * (x)+2] = ip[2];
			p[4 * (x)+2] = 2;
		}
		else if (ip[2] >= ip[6] && ip[2] <= -2147480000){
			d[4 * (x)+2] = -2147480000;
			p[4 * (x)+2] = 1;
		}
		else if (ip[2] < ip[6]){
			d[4 * (x)+2] = ip[6];
			p[4 * (x)+2] = 4;
		}
		//d(4)
		if (ip[3] >= ip[7] && ip[2] >= -2147480000){
			d[4 * (x)+3] = ip[3];
			p[4 * (x)+3] = 2;
		}
		else if (ip[3] >= ip[7] && ip[3] <= -2147480000){
			d[4 * (x)+3] = -2147480000;
			p[4 * (x)+3] = 1;
		}
		else if (ip[3] < ip[7]){
			d[4 * (x)+3] = ip[7];
			p[4 * (x)+3] = 4;
		}

	}

	printf("Traceback has begun...\n");
	// Traceback to start to find codeword
	int m = 1;
	int pp;
	int k;
	for (k = -LEN - 1; k < -1; k++){
		pp = p[4 * (-k - 1) + m - 1];
		output[-k - 2] = A[4 * (pp - 1) + m - 1];
		m = pp;
	}

	// decoded bits stored in 'output'

	printf("I've reached the end of the program.\n");
	while (1);
	return;
}
