#include "syscall.h"

int main() {
	Write("Execing simulation instance 1\n", sizeof("Execing simulation instance 1\n"), ConsoleOutput);
	Exec("../test/threadtest", sizeof("../test/Exec_Test"));
	
	Write("Execing simulation instance 2\n", sizeof("Execing simulation instance 2\n"), ConsoleOutput);
	Exec("../test/threadtest", sizeof("../test/Exec_Test"));
	
	Exit(0);
}
