#include "syscall.h"

void testFuction(){
	Write("FORK YO\n", 8, ConsoleOutput);
	Exit(0);
}

int main(){
	Exec("../test/testfiles", 17);
	Exec("../test/nnnnnn", 14);

	Fork(testFuction);
}