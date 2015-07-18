#include "syscall.h"

int main(){
	int which;
	int testLock;
	int testCV;
	int testMV;
	
	/* Creating, Acquiring, Releasing a lock */
	testLock = CreateLock("tes");
	printf((int)"Created lock %d.\n", sizeof("Created lock %d.\n"), testLock, 0);
	Acquire(testLock);
	printf((int)"Acquired lock %d.\n", sizeof("Acquired lock %d.\n"), testLock, 0);
	/* Creating, Signalling a CV */
	testCV = CreateCV("testCV Create Destroy");
	printf((int)"Created CV %d.\n", sizeof("Created CV %d.\n"), testCV, 0);
	/* Create MV, Set MV, Destroy MV*/
	testMV = CreateMV("mv", 0);
	printf((int)"Create MV %d.\n", sizeof("Create MV %d.\n"), testMV, 0);
	SetMV(testMV, 5);
	printf((int)"Set MV %d.\n", sizeof("Set MV %d.\n"), testMV, 0);
	Signal(testCV, testLock);
	printf((int)"Signalled CV %d.\n", sizeof("Signalled CV %d.\n"), testCV, 0);
	Release(testLock);
	printf((int)"Released lock %d.\n", sizeof("Released lock $d.\n"), testLock, 0);
	
	Exit(0);
	
	/*ASDASDASD*/
}
