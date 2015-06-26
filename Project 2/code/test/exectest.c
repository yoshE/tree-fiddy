#include "syscall.h"

int main(){
	Write(" Testing Exec with valid file\n", sizeof(" Testing Exec with valid file\n"), ConsoleOutput);
	Exec("../test/Exec_Test", sizeof("../test/Exec_Test"));
	Write(" Testing Exec with invalid file\n", sizeof(" Testing Exec with invalid file\n"), ConsoleOutput);
	Exec("../test/nnnnnn", sizeof("../test/nnnnnn"));
	
	Exit(0);
}