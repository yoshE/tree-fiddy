#include "syscall.h"

int testLock;
int testLock2;

int main(){
	int which;
	/*Creating and destroying a lock*/
	testLock = CreateLock("test");
	printf((int)"Created lock.\n", sizeof("Created lock.\n"), 0, 0);
	DestroyLock(testLock);
	printf((int)"Destroyed lock.\n", sizeof("Destroyed lock.\n"), 0, 0);
	Exit(0);
}
