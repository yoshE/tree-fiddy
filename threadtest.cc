// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "threadtest.h"
#include <stdlib.h>
#include <iostream>

#define BAGGAGE_COUNT 2
#define BAGGAGE_WEIGHT 30
#define AIRLINE_COUNT 3

using namespace std;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
	//set up Passenger
	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		liaisonLine[i] = 0;
	}
	srand (time(NULL));
	/*liaisonLineCV = new list<Condition>();
	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		char* name = "Liaison Line CV" + i;
		Condition *tempCondition = new Condition(name);
		liaisonLineCV->push_front(tempCondition);
	}*/

	//liaisonLineCV = new List;
	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		char* name = "Liaison Line CV" + i;
		Condition *tempCondition = new Condition(name);
		liaisonLineCV[i] = tempCondition;
	}
	char* lockName = "Passenger Line Lock";
	liaisonLineLock = new Lock(lockName);

	//end set up

    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

Passenger::Passenger(int n){
	name = n;
	airline = rand() % AIRLINE_COUNT;
	economy = true;
	if(rand() % 2 == 1){
		economy = false;
	}
	baggageCount = rand() % 2 + BAGGAGE_COUNT;
	for(int i = 0; i < baggageCount; i++){
		baggageWeight[i] = rand() % 31 + BAGGAGE_WEIGHT;
	}
	
}

Passenger::~Passenger(){
}

//void randomAccessHelper(int 

void Passenger::ChooseLiaisonLine(){
	liaisonLineLock->Acquire();
	myLine = 0;
	for(int i = 1; i < LIAISONLINE_COUNT; i++){
		if(liaisonLine[i] < liaisonLine[myLine]){
			myLine = i;
		}
	}
	if(liaisonLine[myLine] > 0){
		liaisonLineCV[myLine]->Wait(liaisonLineLock);
	}
	liaisonLineLock->Release();
	//now go up to Liaison
}