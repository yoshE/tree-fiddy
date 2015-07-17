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
	     A[i][j] = Dim-i;
	     B[i][j] = Dim-j;
	     C[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
		 C[i][j] += A[i][k] * B[k][j];

    printf("Forked MatMult2 Answer = %d\n", sizeof("Forked MatMult2 Answer = %d\n"), C[Dim-1][Dim-1], 0);		/* and then we're done */
}

void execSingleProcess() {
	Exec("../test/matmult", sizeof("../test/matmult"));
}

int main() {
	Write("forkSingleProcess()\n", sizeof("forkSingleProcess()\n"), ConsoleOutput);
	
	Fork(matmult);
	Fork(matmult2);
	
	Exit(0);
}	