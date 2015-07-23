#include "syscall.h"

int main() {
	Write("Execing 2 different matmults\n", sizeof("Execing 2 different matmults\n"), ConsoleOutput);
	
	Exec("../test/matmult", sizeof("../test/matmult"));
	Exec("../test/matmult2", sizeof("../test/matmult2"));
	
	Exit(0);
}