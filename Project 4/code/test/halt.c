/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics! 
 */

#include "../userprog/syscall.h"
int a[3];
int b, c, d, e;

int
main()
{
	testLock();
	testCV();
	
    Yield();
    /* not reached */
}

int testLock(){
	/*Test Lock Creation */
	b = CreateLock("");
	if(b >= 0){
		/* Tests the printf Syscall! */
		printf((int)" Testing lock creation...Success(This was printed with printf syscall)\n", sizeof(" Testing lock creation...Success(This was printed with printf syscall)\n"),0,0);
	} else {
		Write(" Testing lock creation...Fail\n", sizeof(" Testing lock creation...Fail\n"), ConsoleOutput );
	}
	
	/*Test Acquiring Lock */
	Acquire(b);
	Release(b);
	
	/*Test Destroy Lock */
	Write(" Destroy Lock Now!\n", sizeof(" Destroy Lock Now!\n"), ConsoleOutput );
	DestroyLock(b);
	Write(" Testing lock destroy again will fail\n", sizeof(" Testing lock destroy again will fail\n"), ConsoleOutput );
	if(b >= 0) DestroyLock(b);
	
	/*Test Acquire with negative index*/
	c = -1;
	Write(" Testing Acquire with -1\n", sizeof(" Testing Acquire with -1\n"), ConsoleOutput );
	Acquire(c);
	Write(" Testing Release with -1\n", sizeof(" Testing Release with -1\n"), ConsoleOutput );
	Release(c);
	
	/*Test Acquire with index that has no value*/
	c = 10;
	Write(" Testing Acquire with index that doesn't exist\n", sizeof(" Testing Acquire with index that doesn't exist\n"), ConsoleOutput );
	Acquire(c);
	Write(" Testing Release with index that doesn't exist\n", sizeof(" Testing Release with index that doesn't exist\n"), ConsoleOutput );
	Release(c);
	return;
}

int testCV(){
	/*Test CV Creation */
	d = CreateCV("");
	if(d >= 0){
		Write(" Testing CV creation...Pass\n" , sizeof(" Testing CV creation...Pass\n"), ConsoleOutput );
	} else {
		Write(" Testing CV creation...Fail\n", sizeof(" Testing CV creation...Fail\n"), ConsoleOutput );
	}
	
	/*Test Destroy CV*/
	Write(" Destroy CV Now!\n", sizeof(" Destroy CV Now!\n"), ConsoleOutput );
	DestroyCV(d);
	Write(" Testing CV destroy again will fail\n", sizeof(" Testing CV destroy again will fail\n"), ConsoleOutput );
	if(d >= 0) DestroyLock(d);
	return;
}
