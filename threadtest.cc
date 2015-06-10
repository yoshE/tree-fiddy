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
#define AIRLINE_COUNT 3 // This is hard coded - DONT CHANGE
#define CHECKIN_COUNT 5

using namespace std;
LiaisonOfficer *liaisonOfficers[LIAISONLINE_COUNT];
CheckInOfficer *CheckIn1[CHECKIN_COUNT];
CheckInOfficer *CheckIn2[CHECKIN_COUNT];
CheckInOfficer *CheckIn3[CHECKIN_COUNT];

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
	
	for (int i = 0; i < AIRLINE_COUNT; i++){
		for (int y = 0; y < CHECKIN_COUNT; y++){
			char* name = "Check In Officer " + y;
			CheckInOfficer *tempCheckIn = new CheckInOfficer(name, y, i);
			switch (i){
				case 1:
					CheckIn1[y] = tempCheckIn;
					break;
				case 2:
					CheckIn1[y] = tempCheckIn;
					break;
				case 3:
					CheckIn1[y] = tempCheckIn;
					break;
			}
		}
	}
	
	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		char* name = "Liaison Line CV " + i;
		Condition *tempCondition = new Condition(name);
		liaisonLineCV[i] = tempCondition;
		char* name2 = "Liaison Line Lock " + i;
		Lock *tempLock = new Lock(name2);
		liaisonLineLocks[i] = tempLock;
		char* name3 = "Liaison Officer " + i;
		LiaisonOfficer *tempLiaison = new LiaisonOfficer(name3, i);
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
		liaisonLine[myLine] = liaisonLine[myLine] + 1;
		liaisonLineCV[myLine]->Wait(liaisonLineLock);
	}
	if (liaisonLine[myLine] > 0) liaisonLine[myLine] = liaisonLine[myLine] - 1; //Passenger left the line
	liaisonLineLock->Release();
	liaisonLineLocks[myLine]->Acquire(); // New lock needed for liaison interaction
	liaisonOfficers[myLine]->setPassengerBaggageCount(this->getBaggageCount(), this); // Informs Liaison of baggage count and which passenger
	liaisonLineCV[myLine]->Signal(liaisonLineLocks[myLine]); // Wakes up Liaison Officer
	liaisonLineCV[myLine]->Wait(liaisonLineLocks[myLine]); // Goes to sleep until Liaison finishes assigning airline
	liaisonOfficers[myLine]->PassengerLeaving(); // Inform liaison passenger is leaving
	liaisonLineLocks[myLine]->Release(); // Passenger is now leaving to go to airline checking
	
    ChooseCheckIn(); // Passenger is now going to airline check In
}

void Passenger::ChooseCheckIn(){
	CheckInLock->Acquire();
	if(!this->getClass()){
		myLine = 0; //Executive line is 0
	} else {
		myLine = 1;
		for(int i = 1; i < CHECKIN_COUNT-1; i++){ // There are 4 economy and 1 executive queues
			switch (this->getAirline()){
				case 1:
					if(CheckIn1[i] < CheckIn1[myLine]) myLine = i;
					break;
				case 2:
					if(CheckIn2[i] < CheckIn2[myLine]) myLine = i;
					break;
				case 3:
					if(CheckIn3[i] < CheckIn3[myLine]) myLine = i;
					break;
			}
		}
	}
	switch (this->getAirline()){
		case 1:
			if(CheckIn1[myLine] > 0){
				CheckIn1[myLine] = CheckIn1[myLine] + 1;
				CheckIn1CV[myLine]->Wait(CheckInLock);
			}
			break;
		case 2:
			if(CheckIn2[myLine] > 0){
				CheckIn2[myLine] = CheckIn2[myLine] + 1;
				CheckIn2CV[myLine]->Wait(CheckInLock);
			}
			break;
		case 3:
			if(CheckIn3[myLine] > 0){
				CheckIn3[myLine] = CheckIn3[myLine] + 1;
				CheckIn3CV[myLine]->Wait(CheckInLock);
			}
			break;
	}
	if (this->getAirline() == 1){
		if (CheckIn1[myLine] > 0) CheckIn1[myLine] = CheckIn1[myLine] - 1;
		CheckInLock->Release();
	} else if (this->getAirline() == 2){
		if (CheckIn2[myLine] > 0) CheckIn2[myLine] = CheckIn2[myLine] - 1;
		CheckInLock->Release();
	}else {
		if (CheckIn3[myLine] > 0) CheckIn3[myLine] = CheckIn3[myLine] - 1;
		CheckInLock->Release();
	}	
}

//----------------------------------------------------------------------
// Liaison Officer
//----------------------------------------------------------------------
LiaisonOfficer::LiaisonOfficer(char* debugName, int i){
	info.name = debugName;
	info.passengerCount = 0;
	info.baggageCount.clear();
	info.number = i;
}

LiaisonOfficer::~LiaisonOfficer(){}
char* LiaisonOfficer::getName() {
	char* s = new char[strlen(info.name)];
	strcpy(s, info.name);
	return s;
}
int LiaisonOfficer::getPassengerCount() {return info.passengerCount;} // For manager to get passenger headcount
int LiaisonOfficer::getPassengerBaggageCount(int n) {return info.baggageCount.at(n);} // For manager to get passenger bag count

void LiaisonOfficer::setPassengerBaggageCount(int n, Passenger* x) { // Passenger Liaison Interaction
	info.airline = rand() % AIRLINE_COUNT; // Randomly generate airline for passenger
	info.passengerCount += 1;
	info.baggageCount.at(info.passengerCount) = n; // Appends Passenger Bag info to Baggage Count Vector
	x->setAirline(info.airline); // Informs the passenger of their airline
	liaisonLineCV[info.number]->Signal(liaisonLineLocks[info.number]); // Wakes up passenger
	liaisonLineCV[info.number]->Wait(liaisonLineLocks[info.number]); // Goes to sleep until next passenger starts interaction
}
void LiaisonOfficer::PassengerLeaving(){} // Passenger informed Liaison they are leaving so Liaison stays asleep

//----------------------------------------------------------------------
// Check In Staff
//----------------------------------------------------------------------
CheckInOfficer(char* deBugName, int i, int y){
	info.name = deBugName;
	info.number = i;
	info.airline = y;
	info.passengerCount = 0;
	info.baggageCount.clear();
	info.OnBreak = true;
}

~CheckInOfficer();
char* getName();
bool getBreak(); // For managers to see who is on break
	




