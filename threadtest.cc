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
	
	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		char* name = "Liaison Line CV" + i;
		Condition *tempCondition = new Condition(name);
		liaisonLineCV[i] = tempCondition;
		char* name2 = "Liaison Line Lock" + i;
		Lock *tempLock = new Lock(name2);
		liaisonLineLock[i] = tempLock;
		char* name3 = "Liaison Officer" + i;
		LiaisonOfficer *tempLiaison = new LiaisonOfficer(name3);
		liaisonOfficers[i] = tempLiaison;
	}
	char* lockName = "Passenger Line Lock";
	liaisonLineLock = new Lock(lockName);

	//end set up

    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// Passenger
//----------------------------------------------------------------------
Passenger::Passenger(int n){
	name = n;
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

void Passenger::setAirline(int n){
	airline = n;
}

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
		liaisonLine[myLine] = liaisonLine[myLine] + 1;
	}
	if (liaisonLine[myLine] > 0) liaisonLine[myLine] = liaisonLine[myline] - 1; //Passenger left the line
	liaisonLineLock->Release();
	liaisonLineLock[myLine]->Acquire();
	liaisonOfficers[myLine]->setBaggageCount(this.getBaggageCount(), this);
	liaisonLineCV[myLine]->Signal(liaisonLineLock[myLine]);
	liaisonLineCV[myLine]->Wait(liaisonLineLock[myLine]);
	liaisonOfficers[myLine]->PassengerLeaving();
	liaisonLineCV[myLine]->Signal(liaisonLineLock[myLine]);
	liaisonLineLock[myline]->Release();
	// Go to Airline
}

//----------------------------------------------------------------------
// Liaison Officer
//----------------------------------------------------------------------
LiaisonOfficer(char* debugName){
	Liaison info;
	info.name = debugName;
	info.passengerCount = 0;
	info.baggageCount.clear();
}

~LiaisonOfficer(){}
int getName() {return info.name;}
int getPassengerCount() {return info.passengerCount;} // For manager to get passenger headcount
int getPassengerBaggageCount(int n) {return info.baggageCount.at(n)}; // For manager to get passenger bag count
void setPassengerBaggageCount(int n, Passenger x) {
	info.airline = rand() % AIRLINE_COUNT; // Randomly generate airline for passenger
	info.passengerCount += 1;
	info.baggageCount.at(info.getPasengerCount()) = n;
	x.setAirline(info.airline);
	liaisonLineCV[myLine]->Signal(liaisonLinkLock[myLine]);
	liaisonLineCV[myLine]->Wait(liaisonLinkLock[myLine]);
}
void PassengerLeaving(){
	liaisonLineCV[myLine]->Wait(liaisonLinkLock[myLine]);
}

//----------------------------------------------------------------------
// Check In Staff
//----------------------------------------------------------------------





