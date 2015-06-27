#include "syscall.h"

int main() {
	Write("Execing simulation instance 1\n", sizeof("Execing simulation instance 1\n"), ConsoleOutput);
	Exec("../test/Airport", sizeof("../test/Airport"));
	
	Write("Execing simulation instance 2\n", sizeof("Execing simulation instance 2\n"), ConsoleOutput);
	Exec("../test/Airport", sizeof("../test/Airport"));
	
	Exit(0);
}
