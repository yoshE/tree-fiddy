#include "syscall.h"

int num = 10;

void functionA() {
	Write("Test\n", 5, ConsoleOutput);
	Exit(0);
}

void functionB() {
	Write("Test2\n", 6, ConsoleOutput);
	Exit(0);
}

void functionC() {
	Write("Test3\n", 6, ConsoleOutput);
	Exit(0);
}

void functionD() {
	Write("Test4\n", 6, ConsoleOutput);
	Exit(0);
}

int main() {

	Write("Hellooooooo\n", 12, ConsoleOutput);
	Fork(functionA);
	Fork(functionB);
	Fork(functionC);
	Fork(functionD);

}

/*
int main() {
  OpenFileId fd;
  int bytesread;
  char buf[20];

    Create("testfile", 8);
    fd = Open("testfile", 8);

    Write("testing a write\n", 16, fd );
    Close(fd);


    fd = Open("testfile", 8);
    bytesread = Read( buf, 100, fd );
    Write( buf, bytesread, ConsoleOutput );
    Close(fd);
}
*/