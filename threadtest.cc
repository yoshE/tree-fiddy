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
CheckInOfficer *CheckIn[CHECKIN_COUNT*AIRLINE_COUNT];
LiaisonPassengerInfo * LPInfo = new LiaisonPassengerInfo[LIAISONLINE_COUNT];
CheckInPassengerInfo * CPInfo = new CheckInPassengerInfo[CHECKIN_COUNT*AIRLINE_COUNT+AIRLINE_COUNT];

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
	
	//For Economy Class
	for (int i = 0; i < AIRLINE_COUNT; i++){
		for (int y = 0; y < CHECKIN_COUNT; y++){
			char* name = "Check In Officer " + y;
			CheckInOfficer *tempCheckIn = new CheckInOfficer(name, y, i);
			CheckIn[(y+i)+AIRLINE_COUNT*i] = tempCheckIn;
		}
	}
	//For Exec Line
	for (int i = 0; i<AIRLINE_COUNT; i++){
		name = "Check In Officer " + y;
		CheckInOfficer *tempCheckIn = new CheckInOfficer(name, y, i);
		CheckIn[CHECKING_COUNT*AIRLINE_COUNT + i] = tempCheckIn;
	}
	
	
	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		char* name = "Liaison Line CV " + i;
		Condition *tempCondition = new Condition(name);
		liaisonLineCV[i] = tempCondition;
		char* name4 = "Liaison Officer CV " + i;
		tempCondition = new Condition(name4);
		liaisonOfficerCV[i] = tempCondition;
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
	ChooseLiaisonLine();
}

Passenger::~Passenger(){
}

void Passenger::setAirline(int n){
	airline = n;
}

void Passenger::setSeat(int n){
	seat = n;
}

void Passenger::ChooseLiaisonLine(){
	liaisonLineLock->Acquire();
	myLine = 0;
	for(int i = 1; i < LIAISONLINE_COUNT; i++){
		if(liaisonLine[i] < liaisonLine[myLine]){
			myLine = i;
		}
	}
	liaisonLine[myLine] = liaisonLine[myLine] + 1;
	if(liaisonLine[myLine] > 0){
		liaisonLineCV[myLine]->Wait(liaisonLineLock);
	}
	liaisonLineLock->Release();
	liaisonLineLocks[myLine]->Acquire(); // New lock needed for liaison interaction
	LPInfo.baggageCount = baggageCount; // Adds baggage Count to shared struct array
	liaisonOfficerCV[myLine]->Signal(liaisonLineLocks[myLine]); // Wakes up Liaison Officer
	liaisonOfficerCV[myLine]->Wait(liaisonLineLocks[myLine]); // Goes to sleep until Liaison finishes assigning airline
	airline = LPInfo.airline;
	if (liaisonLine[myLine] > 0) liaisonLine[myLine] = liaisonLine[myLine] - 1; //Passenger left the line
	liaisonLineLocks[myLine]->Release(); // Passenger is now leaving to go to airline checking
	
    ChooseCheckIn(); // Passenger is now going to airline check In
}

void Passenger::ChooseCheckIn(){
	CheckInLock->Acquire();
	if(!this->getClass()){
		myLine = CHECKIN_COUNT*AIRLINE_COUNT+airline-1; // Executive Line is 15 (Airline 1),16 (Airline 2),17 (Airline 3)
	} else {
		myLine = 0; // Normal lines are 0-14 (Airline 1 is 0-4, Airline 2 is 5-9, Airline 3 is 10-14
		for (int i = 1; i < CHECKIN_COUNT*AIRLINE_COUNT; i++){
			if (CheckIn[i] < CheckIn[myLine]) myLine = i;	
		}
	}
	CheckIn[myLine] = CheckIn[myLine] + 1;
	CheckInCV[myLine]->Wait(CheckInLock);
	if(CheckIn[myLine] > 0) CheckIn[myLine] = CheckIn[myLine] - 1;
	CheckInLock->Release();
	CheckInLocks->Acquire();
	CPInfo[myLine].baggageCount = baggageCount;
	for (int i = 0; i < baggageCount; i++){
		CPInfo[myLine].bag.push_back(Baggage());
		CPInfo[myLine].bag[0].weight = PASSENGER WEIGHT IS;
	}
	CheckInOfficerCV[myLine]->Signal(CheckInLocks[myLine]);
	CheckInOfficerCV[myLine]->Wait(CheckInLocks[myLine]);
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

void DoWork(){
	while(true){
		if (LiaisonLine[info.number].size() > 0){
			liaisionLineLock->Acquire();
			LiaisonLineCV[info.number]->Signal(liaisonLineLock);
			LiaisonLineLocks[info.number]->Acquire();
			liaisonLineLock->Release();
			LiaisonLineCV[info.number]->Wait(liaisionLineLocks[info.number]);
			// Passenger has given bag Count info and woken up the Liaison Officer
			info.passengerCount += 1;
			info.baggageCount.at(info.passengerCount) = n;
			info.airline = rand() % AIRLINE_COUNT;
			LPInfo.airline = info.airline;
			liaisonOfficerCV[info.number]->Signal(liaisonLineLocks[info.number]); // Wakes up passenger
			liaisonOfficerCV[info.number]->Wait(liaisonLineLocks[info.number]); // Goes to sleep until next passenger starts interaction
		}
	}
}

//----------------------------------------------------------------------
// Check In Staff
//----------------------------------------------------------------------
CheckInOfficer::CheckInOfficer(char* deBugName, int i, int y){
	info.name = deBugName;
	info.number = i;
	info.airline = y;
	info.passengerCount = 0;
	info.baggageCount.clear();
	info.OnBreak = true;
	info.work = true;
}

CheckInOfficer::~CheckInOfficer(){}
char* CheckInOfficer::getName(){
	char* s = new char[strlen(info.name)];
	strcpy(s, info.name);
	return s;
}
bool CheckInOfficer::getBreak(){ // For managers to see who is on break
	return info.OnBreak;
}
int CheckInOfficer::getAirline(){return info.airline;}
int CheckInOfficer::getNumber() {return info.number;}

void CheckInOfficer::DoWork(){
	while(!OnBreak){
		if(this->getAirline() == 1){
			if(CheckIn1[0].size() > 0){
				CheckIn1CV[0]->Signal(CheckInLock);
				CheckInLocks[this->getNumber()]->Acquire();
				CheckInOfficer1CV[0]->Wait(CheckInLocks[this->getNumber()]);
			} else if (CheckIn1[this->getNumber()].size() > 0) {
				CheckIn1CV[this->getNumber()]->Signal(CheckInLock);
				CheckInLocks[this->getNumber()]->Acquire();
				CheckInOfficer1CV[this->getNumber()]->Wait(CheckInLocks[this->getNumber()]);
			} else {
				OnBreak = true;
			}
		}else if(this->getAirline() == 2){
			if(CheckIn2[0].size() > 0){
				CheckIn2CV[0]->Signal(CheckInLock);
				CheckInLocks[this->getNumber()]->Acquire();
				CheckInOfficer2CV[0]->Wait(CheckInLocks[this->getNumber()]);
			} else if (CheckIn2[this->getNumber()].size() > 0) {
				CheckIn2CV[this->getNumber()]->Signal(CheckInLock);
				CheckInLocks[this->getNumber()]->Acquire();
				CheckInOfficer2CV[this->getNumber()]->Wait(CheckInLocks[this->getNumber()]);
			}else {
				OnBreak = true;
			}
		}else{
			if(CheckIn3[0].size() > 0){
				CheckIn3CV[0]->Signal(CheckInLock);
				CheckInLocks[this->getNumber()]->Acquire();
				CheckInOfficer3CV[0]->Wait(CheckInLocks[this->getNumber()]);
			} else if (CheckIn3[this->getNumber()].size() > 0) {
				CheckIn3CV[this->getNumber()]->Signal(CheckInLock);
				CheckInLocks[this->getNumber()]->Acquire();
				CheckInOfficer3CV[this->getNumber()]->Wait(CheckInLocks[this->getNumber()]);
			}else {
				OnBreak = true;
			}
		}	
	}
}

void CheckInOfficer::setBaggageCount(struct y, passenger* x){ // ask question if the function is run then the thread is returned to its sleep position
	info.passengerCount += 1;
	this->setBag(y);
	x->setSeat(int n);//array of bools
	CheckInOfficerCV[info.number]->Signal(CheckInLocks[info.number]);
	CheckInOfficerCV[info.number]->Wait(CheckInLocks[info.number]);
}

void CheckInOfficer::setBag(struct x){
	bag.count = x.count;
	bag.baggageWeight[3] = x.baggageWeight[3];
}

// --------------------------------------------------
// Test Suite
// --------------------------------------------------

// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the
                                  // lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the 
                                  // lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the
                                  // lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock t1_l1("t1_l1");		  // the lock tested in Test 1

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
    t1_l1.Acquire();
    t1_s1.V();  // Allow t1_t2 to try to Acquire Lock
 
    printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
	    t1_l1.getName());
    t1_s3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {

    t1_s1.P();	// Wait until t1 has the lock
    t1_s2.V();  // Let t3 try to acquire the lock

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
	    t1_l1.getName());
    for (int i = 0; i < 10; i++)
	;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {

    t1_s2.P();	// Wait until t2 is ready to try to acquire the lock

    t1_s3.V();	// Let t1 do it's stuff
    for ( int i = 0; i < 3; i++ ) {
	printf("%s: Trying to release Lock %s\n",currentThread->getName(),
	       t1_l1.getName());
	t1_l1.Release();
    }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");		// For mutual exclusion
Condition t2_c1("t2_c1");	// The condition variable to test
Semaphore t2_s1("t2_s1",0);	// To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
                                  // done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Signal(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
    t2_s1.V();	// release t2_t2
    t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
    t2_s1.P();	// Wait for t2_t1 to be done with the lock
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Wait(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");		// For mutual exclusion
Condition t3_c1("t3_c1");	// The condition variable to test
Semaphore t3_s1("t3_s1",0);	// To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
    t3_l1.Acquire();
    t3_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Wait(&t3_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
    t3_l1.Release();
    t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {

    // Don't signal until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
	t3_s1.P();
    t3_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Signal(&t3_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
    t3_l1.Release();
    t3_done.V();
}
 
// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");		// For mutual exclusion
Condition t4_c1("t4_c1");	// The condition variable to test
Semaphore t4_s1("t4_s1",0);	// To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
    t4_l1.Acquire();
    t4_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Wait(&t4_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
    t4_l1.Release();
    t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {

    // Don't broadcast until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
	t4_s1.P();
    t4_l1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Broadcast(&t4_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
    t4_l1.Release();
    t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");		// For mutual exclusion
Lock t5_l2("t5_l2");		// Second lock for the bad behavior
Condition t5_c1("t5_c1");	// The condition variable to test
Semaphore t5_s1("t5_s1",0);	// To make sure t5_t2 acquires the lock after
                                // t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
    t5_l1.Acquire();
    t5_s1.V();	// release t5_t2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t5_l1.getName(), t5_c1.getName());
    t5_c1.Wait(&t5_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
    t5_s1.P();	// Wait for t5_t1 to get into the monitor
    t5_l1.Acquire();
    t5_l2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t5_l2.getName(), t5_c1.getName());
    t5_c1.Signal(&t5_l2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l2.getName());
    t5_l2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
//	 4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------
void TestSuite() {
    Thread *t;
    char *name;
    int i;
    
    // Test 1

    printf("Starting Test 1\n");

    t = new Thread("t1_t1");
    t->Fork((VoidFunctionPtr)t1_t1,0);

    t = new Thread("t1_t2");
    t->Fork((VoidFunctionPtr)t1_t2,0);

    t = new Thread("t1_t3");
    t->Fork((VoidFunctionPtr)t1_t3,0);

    // Wait for Test 1 to complete
    for (  i = 0; i < 2; i++ )
	t1_done.P();

    // Test 2

    printf("Starting Test 2.  Note that it is an error if thread t2_t2\n");
    printf("completes\n");

    t = new Thread("t2_t1");
    t->Fork((VoidFunctionPtr)t2_t1,0);

    t = new Thread("t2_t2");
    t->Fork((VoidFunctionPtr)t2_t2,0);

    // Wait for Test 2 to complete
    t2_done.P();

    // Test 3

    printf("Starting Test 3\n");

    for (  i = 0 ; i < 5 ; i++ ) {
	name = new char [20];
	sprintf(name,"t3_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t3_waiter,0);
    }
    t = new Thread("t3_signaller");
    t->Fork((VoidFunctionPtr)t3_signaller,0);

    // Wait for Test 3 to complete
    for (  i = 0; i < 2; i++ )
	t3_done.P();

    // Test 4

    printf("Starting Test 4\n");

    for (  i = 0 ; i < 5 ; i++ ) {
	name = new char [20];
	sprintf(name,"t4_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t4_waiter,0);
    }
    t = new Thread("t4_signaller");
    t->Fork((VoidFunctionPtr)t4_signaller,0);

    // Wait for Test 4 to complete
    for (  i = 0; i < 6; i++ )
	t4_done.P();

    // Test 5

    printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
    printf("completes\n");

    t = new Thread("t5_t1");
    t->Fork((VoidFunctionPtr)t5_t1,0);

    t = new Thread("t5_t2");
    t->Fork((VoidFunctionPtr)t5_t2,0);

}
