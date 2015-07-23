#include "syscall.h"

int main(){
	int which;
	int testLock;
	int testCV;
	int testMV;
	
	/* Creating, Acquiring, Releasing, and Destroying a lock */
	testLock = CreateLock("tes");
	printf((int)"Created lock %d.\n", sizeof("Created lock %d.\n"), testLock, 0);
	Acquire(testLock);
	printf((int)"Acquired lock %d.\n", sizeof("Acquired lock %d.\n"), testLock, 0);
	/* Creating, Waiting, Signalling and Destroying a CV */
	testCV = CreateCV("testCV Create Destroy");
	printf((int)"Created CV %d.\n", sizeof("Created CV %d.\n"), testCV, 0);
	/* Create MV, Get MV, Destroy MV*/
	testMV = CreateMV("mv", 0);
	printf((int)"Create MV %d.\n", sizeof("Create MV %d.\n"), testMV, 0);
	GetMV(testMV);
	printf((int)"Get MV %d.\n", sizeof("Get MV %d.\n"), testMV, 0);
	Wait(testCV, testLock);
	printf((int)"Waiting CV %d.\n", sizeof("Waiting CV %d.\n"), testCV, 0);
	GetMV(testMV);
	printf((int)"Get MV %d.\n", sizeof("Get MV %d.\n"), testMV, 0);
	Release(testLock);
	printf((int)"Released lock %d.\n", sizeof("Released lock $d.\n"), testLock, 0);
	DestroyCV(testCV);
	printf((int)"Destroyed CV.\n", sizeof("Destroyed CV.\n"), 0, 0);
	DestroyLock(testLock);
	printf((int)"Destroyed lock.\n", sizeof("Destroyed lock.\n"), 0, 0);
	DestroyMV(testMV);
	printf((int)"Destroyed MV.\n", sizeof("Destroyed MV.\n"), 0, 0);
	
	Exit(0);
}
