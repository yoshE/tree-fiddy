#include "syscall.h"

int main() {
	Write("Exec Test Pass!\n", sizeof("Exec Test Pass!\n"), ConsoleOutput );
	Exit(-1);
}
