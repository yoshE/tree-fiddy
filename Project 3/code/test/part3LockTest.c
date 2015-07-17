#include "syscall.h"

int main(){
	int which;
	int testLock;
	int testCV;
	
	/*Creating and destroying a lock*/
	printf((int)"SUP\n", sizeof("SUP\n"), 0, 0);
	testLock = CreateLock("testLock Create Destroy");
	printf((int)"Created lock %d.\n", sizeof("Created lock %d.\n"), testLock, 0);
	/* Creating, Acquiring, Releasing, and Destroying a lock */
	testLock = CreateLock("testLock Acquire Release");
	printf((int)"Created lock %d.\n", sizeof("Created lock %d.\n"), testLock, 0);
	Acquire(testLock);
	printf((int)"Acquired lock %d.\n", sizeof("Acquired lock %d.\n"), testLock, 0);
	/* Creating, Signalling, Broadcasting, and Destroying a CV */
	testCV = CreateCV("testCV Create Destroy");
	printf((int)"Created CV %d.\n", sizeof("Created CV %d.\n"), testCV, 0);
	Signal(testCV, testLock);
	printf((int)"Signalled CV %d.\n", sizeof("Signalled CV %d.\n"), testCV, 0);
	Broadcast(testCV, testLock);
	Release(testLock);
	printf((int)"Released lock %d.\n", sizeof("Released lock $d.\n"), testLock, 0);
	DestroyCV(testCV);
	printf((int)"Destroyed CV.\n", sizeof("Destroyed CV.\n"), 0, 0);
	DestroyLock(testLock);
	printf((int)"Destroyed lock.\n", sizeof("Destroyed lock.\n"), 0, 0);	
	
	Exit(0);
/* changed file so i can push */
}
