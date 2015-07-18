#include "syscall.h"

int main(){
	int which;
	int testLock;
	int testCV;
	
	/* Creating, Acquiring, Releasing a lock */
	testLock = CreateLock("testLockAcquireRelease");
	printf((int)"Created lock %d.\n", sizeof("Created lock %d.\n"), testLock, 0);
	Acquire(testLock);
	printf((int)"Acquired lock %d.\n", sizeof("Acquired lock %d.\n"), testLock, 0);
	/* Creating, Signalling a CV */
	testCV = CreateCV("testCV Create Destroy");
	printf((int)"Created CV %d.\n", sizeof("Created CV %d.\n"), testCV, 0);
	Signal(testCV, testLock);
	printf((int)"Signalled CV %d.\n", sizeof("Signalled CV %d.\n"), testCV, 0);
	Release(testLock);
	printf((int)"Released lock %d.\n", sizeof("Released lock $d.\n"), testLock, 0);
	
	Exit(0);
	
	/*ASDASDASD*/
}
