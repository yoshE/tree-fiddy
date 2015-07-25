#include "syscall.h"

int lock, cv;

void func1(){
	Write("func1 forked and running\n", sizeof("func1 forked and running\n"), ConsoleOutput );

    Acquire(lock);
    Write("func1 acquired lock\n", sizeof("func1 acquired lock\n"), ConsoleOutput );
    Signal(cv, lock);

    Write("func1 Exiting with exit status 0\n", sizeof("func1 Exiting with exit status 0\n"), ConsoleOutput );

    Release(lock);
    Yield();
    Exit(0);
}

void main(){
    lock = CreateLock("lock", 1);
    cv = CreateCV("condition", 1);
    Acquire(lock);

    Fork(func1);

    Write("main forked func1\n", sizeof("main forked func1\n"), ConsoleOutput );
    Wait(cv, lock);
    Write("main got signaled\n", sizeof("main got signaled\n"), ConsoleOutput );
    Release(lock);
    Write("main released lock\n", sizeof("main released lock\n"), ConsoleOutput );
    Yield();
    Exit(0);
}
