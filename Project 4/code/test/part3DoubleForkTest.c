#include "syscall.h"

#define Dim 	20	/* sum total of the arrays doesn't fit in 
			 * physical memory 
			 */

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];

int D[Dim][Dim];
int E[Dim][Dim];
int F[Dim][Dim];

int matmult() {
	int i, j, k;

    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
	for (j = 0; j < Dim; j++) {
	     A[i][j] = i;
	     B[i][j] = j;
	     C[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
		 C[i][j] += A[i][k] * B[k][j];

    printf("Forked MatMult Answer = %d\n", sizeof("Forked MatMult Answer = %d\n"), C[Dim-1][Dim-1], 0);		/* and then we're done */
}

int matmult2() {
	int i, j, k;

    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
	for (j = 0; j < Dim; j++) {
	     D[i][j] = Dim-i;
	     E[i][j] = Dim-j;
	     F[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
		 F[i][j] += D[i][k] * E[k][j];

    printf("Forked MatMult2 Answer = %d\n", sizeof("Forked MatMult2 Answer = %d\n"), F[Dim-1][Dim-1], 0);		/* and then we're done */
}

int main() {
	Write("Forking 2 different matmult functions\n", sizeof("Forking 2 different matmult functions\n"), ConsoleOutput);
	
	Fork(matmult);
	Fork(matmult2);
	
	Exit(0);
}	