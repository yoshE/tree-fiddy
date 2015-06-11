// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "threadtest.h"
#include <stdlib.h>
#include <iostream>

#define BAGGAGE_COUNT 2		// Passenger starts with 2 baggages and will randomly have one more
#define BAGGAGE_WEIGHT 30		// Baggage weight starts at 30 and can have 0-30 more lbs added randomly
#define AIRLINE_COUNT 3 		// Number of airlines
#define CHECKIN_COUNT 5		// Number of CheckIn Officers

using namespace std;
LiaisonOfficer *liaisonOfficers[LIAISONLINE_COUNT];		// Array of Liaison Officers
CheckInOfficer *CheckIn[CHECKIN_COUNT*AIRLINE_COUNT];		// Array of CheckIn Officers
SecurityOfficer *Security[SCREEN_COUNT];		// Array of Security Officers
LiaisonPassengerInfo * LPInfo = new LiaisonPassengerInfo[LIAISONLINE_COUNT];		// Array of Structs that contain info from passenger to Liaison
CheckInPassengerInfo * CPInfo = new CheckInPassengerInfo[CHECKIN_COUNT*AIRLINE_COUNT+AIRLINE_COUNT];		// Array of Structs that contain info from pasenger to CheckIn
ScreenPassengerInfo * SPInfo = new ScreenPassengerInfo[SCREEN_COUNT+1];
SecurityScreenInfo * SSInfo = new SecurityScreenInfo[SCREEN_COUNT];

deque<Baggage> conveyor;		// Conveyor queue that takes bags from the CheckIn and is removed by Cargo Handlers
int aircraftBaggageCount[AIRLINE_COUNT];		// Number of baggage on a single airline
int aircraftBaggageWeight[AIRLINE_COUNT];		// Weight of baggage on a single airline

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
	printf("Setting up stuff!\n");
	srand (time(NULL));
	//TODO: set up Passenger here

	char* lockName = "Passenger Line Lock";
	liaisonLineLock = new Lock(lockName);
	
	//For Economy Class
	//set up CIS
	for (int i = 0; i < AIRLINE_COUNT; i++){
		for (int y = 0; y < CHECKIN_COUNT; y++){
			int x = (y+i)+AIRLINE_COUNT*i;
			CheckInLine[x] = 0;
			char* name = "Check In Officer " + x;
			CheckInOfficer *tempCheckIn = new CheckInOfficer(x);
			CheckIn[(y+i)+AIRLINE_COUNT*i] = tempCheckIn;
		}
	}
	//For Exec Line
	for (int i = 0; i<AIRLINE_COUNT; i++){
		int x = AIRLINE_COUNT*CHECKIN_COUNT + i;
		CheckInLine[x] = 0;
		char* name = "Check In Officer " + x;
		CheckInOfficer *tempCheckIn = new CheckInOfficer(x);
		CheckIn[CHECKIN_COUNT*AIRLINE_COUNT + i] = tempCheckIn;
	}
	
	
	/*
	//set up Liaison
	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		liaisonLine[i] = 0;
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
	*/
	
	//Set up security officers
	for (int i = 0; i < SCREEN_COUNT; i++){
		char* name = "Security Officer " + i;
		SecurityOfficer *tempSecurity = new SecurityOfficer(i);
		Security[i] = tempSecurity;
	}
	
	//TODO: set up Cargo Handler
	lockName = "Cargo Handler Lock";
	CargoHandlerLock = new Lock(lockName);
	char* cvName = "Cargo Handler CV ";
	CargoHandlerCV = new Condition(cvName);
		
	//end set up

    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// Passenger
//----------------------------------------------------------------------
Passenger::Passenger(int n){		// Constructor
	name = n;
	economy = true;				// Default is economy class
	if(rand() % 4 == 1){		// 25% of being executive class
		economy = false;
	}
	baggageCount = rand() % 2 + BAGGAGE_COUNT;		// Randomly have people with 3 baggage
	for(int i = 0; i < baggageCount; i++){			// Add baggage and their weight to Passenger Baggage Vector
		bags.push_back(Baggage());
		bags[i].weight = rand() % 31 + BAGGAGE_WEIGHT;		// Baggage will weigh from 30 to 60 lb
	}
	ChooseLiaisonLine();
}

Passenger::~Passenger(){
}

void Passenger::ChooseLiaisonLine(){		// Picks a Liaison line, talkes to the Officer, gets airline
	liaisonLineLock->Acquire();		// Acquire lock to find shortest line
	myLine = 0;		
	for(int i = 1; i < LIAISONLINE_COUNT; i++){		// Find shortest line
		if(liaisonLine[i] < liaisonLine[myLine]){
			myLine = i;
		}
	}
	
	printf("Passenger %d chose Liaison %d with a line of length %d\n", name, myLine, liaisonLine[myLine]);
	liaisonLine[myLine] = liaisonLine[myLine] + 1;		// Increment size of line you join
	if(liaisonLine[myLine] > 1){		// If you are waiting, go to sleep until signalled by Liaison Officer
		liaisonLineCV[myLine]->Wait(liaisonLineLock);
	}
	liaisonLineLock->Release();		// Release the lock you acquired from waking up
	std::cout << "About to start convo with Liaison Officer!\n";	// Debugging purposes
	liaisonLineLocks[myLine]->Acquire(); // New lock needed for liaison interaction
	LPInfo->baggageCount = baggageCount; // Adds baggage Count to shared struct array
	printf("Signalling liasonLineLock #%d\n", myLine);
	liaisonOfficerCV[myLine]->Signal(liaisonLineLocks[myLine]); // Wakes up Liaison Officer
	printf("Woke up liaison #%d\n", myLine);
	liaisonOfficerCV[myLine]->Wait(liaisonLineLocks[myLine]); // Goes to sleep until Liaison finishes assigning airline
	airline = LPInfo->airline;		// Gets airline info from Liaison Officer shared struct
	liaisonOfficerCV[myLine]->Signal(liaisonLineLocks[myLine]); // Wakes up Liaison Officer to say I'm leaving
	if (liaisonLine[myLine] > 1) liaisonLine[myLine] = liaisonLine[myLine] - 1; //Passenger left the line
	liaisonLineLocks[myLine]->Release(); // Passenger is now leaving to go to airline checking	

	printf("Passenger %d of Airline %d is directed to the check-in counter\n", name, this->getAirline());
	
// ----------------------------------------------------[ Going to CheckIn ]----------------------------------------

	CheckInLock->Acquire();		// Acquire lock to find shortest line
	if(!this->getClass()){		// If executive class
		myLine = CHECKIN_COUNT*AIRLINE_COUNT+airline-1; // Executive Line is 15 (Airline 1),16 (Airline 2),17 (Airline 3)
	} else { 
		myLine = (airline - 1)*5;		// Economy lines are 0-14 (Airline 1 is 0-4, Airline 2 is 5-9, Airline 3 is 10-14
		for (int i = (airline - 1)*5; i < airline*5; i++){ // Find shortest line for my airline
			if (CheckInLine[i] < CheckInLine[myLine]) myLine = i;	
		}
	}
	CheckInLine[myLine] = CheckInLine[myLine] + 1;		// Increment line when I enter line
	CheckInCV[myLine]->Wait(CheckInLock);		// Wait for CheckIn Officer to signal
	int oldLine = myLine;		// Save old line to decrement it later when you leave
	myLine = CPInfo[myLine].line;		// Sets its line to that of the Officer
	CheckInLock->Release();
	CheckInLocks[myLine]->Acquire();
	CPInfo[myLine].baggageCount = baggageCount;		// Place baggage onto counter (into shared struct)
	for (int i = 0; i < baggageCount; i++){		// Add baggage info to shared struct for CheckIn Officer
		CPInfo[myLine].bag.push_back(Baggage());
		CPInfo[myLine].bag[i].weight = bags[i].weight;
	}
	CheckInOfficerCV[myLine]->Signal(CheckInLocks[myLine]);		// Wake up CheckIn Officer 
	CheckInOfficerCV[myLine]->Wait(CheckInLocks[myLine]);
	seat = CPInfo[myLine].seat;		// Get seat number from shared struct
	CheckInOfficerCV[myLine]->Signal(CheckInLocks[myLine]); // Wakes up CheckIn Officer to say I'm leaving
	if (CheckInLine[oldLine] > 1) CheckInLine[oldLine] = CheckInLine[oldLine] - 1; //Passenger left the line
	CheckInLocks[myLine]->Release(); // Passenger is now leaving to go to screening

// ----------------------------------------------------[ Going to Screening ]----------------------------------------

	ScreenLines->Acquire();
	ScreenLine[0] = ScreenLine[0] + 1;		// Increment length of Screening Line
	ScreenLineCV[0]->Wait(ScreenLines);
	myLine = SPInfo[SCREEN_COUNT+1].line;		// Receive line of which Screening Officer to see
	ScreenLines->Release();
	ScreenLocks[myLine]->Acquire();
	ScreenOfficerCV[myLine]->Signal(ScreenLocks[myLine]);		// Tell Screening Officer that passenger is ready for scan
	ScreenOfficerCV[myLine]->Wait(ScreenLocks[myLine]);
	oldLine = myLine;		// Save old line to signal to Screening you are leaving
	myLine = SPInfo[myLine].line;		// Receive line of which Security Officer to wait for
	ScreenOfficerCV[oldLine]->Signal(ScreenLocks[myLine]);
	if(ScreenLine[0] > 1) ScreenLine[0] = ScreenLine[0] - 1;
	ScreenLocks[oldLine]->Release();
	
// ----------------------------------------------------[ Going to Security ]----------------------------------------



}

//----------------------------------------------------------------------
// Liaison Officer
//----------------------------------------------------------------------
LiaisonOfficer::LiaisonOfficer(int i){		// Constructor
	info.passengerCount = 0;
	info.baggageCount.clear();
	info.number = i;
}
LiaisonOfficer::~LiaisonOfficer(){}
int LiaisonOfficer::getPassengerCount() {return info.passengerCount;} // For manager to get passenger headcount
int LiaisonOfficer::getPassengerBaggageCount(int n) {return info.baggageCount.at(n);} // For manager to get passenger bag count

void LiaisonOfficer::DoWork(){
	printf("Liaison officer is doing work\n");
	while(true){		// Always be running, never go on break
		liaisonLineLock->Acquire();		// Acquire lock for lining up in order to see if there is someone waiting in your line
		if (liaisonLine[info.number] > 0){		// Check if passengers are in your line
			liaisonLineCV[info.number]->Signal(liaisonLineLock);		// Signal them if there are
		}
		liaisonLineLocks[info.number]->Acquire();		
		liaisonLineLock->Release();
		//liaisonLineCV[info.number]->Wait(liaisonLineLocks[info.number]);		// Wait for passenger to give you baggage info
		printf("Waiting on liasonLineLock #%d\n", info.number);
		liaisonOfficerCV[info.number]->Wait(liaisonLineLocks[info.number]);		// Wait for passenger to give you baggage info
		printf("Liaison #%d has AWAKENED!\n", info.number);
		// Passenger has given bag Count info and woken up the Liaison Officer
		info.passengerCount += 1;		// Increment internal passenger counter
		info.baggageCount.push_back(LPInfo[info.number].baggageCount);		// Track baggage Count
		info.airline = rand() % AIRLINE_COUNT;		// Generate airline for passenger
		LPInfo[info.number].airline = info.airline;		// Put airline number in shared struct for passenger
		liaisonOfficerCV[info.number]->Signal(liaisonLineLocks[info.number]); // Wakes up passenger
		liaisonOfficerCV[info.number]->Wait(liaisonLineLocks[info.number]); // Waits for Passenger to say they are leaving
	}
}

//----------------------------------------------------------------------
// Check In Staff
//----------------------------------------------------------------------
CheckInOfficer::CheckInOfficer(int i){
	info.number = i;
	info.airline = i/CHECKIN_COUNT;
	info.passengerCount = 0;
	info.OnBreak = false;
	info.work = true;
}

CheckInOfficer::~CheckInOfficer(){}
bool CheckInOfficer::getBreak() {return info.OnBreak;}
void CheckInOfficer::setBreak() {info.OnBreak = true;}
int CheckInOfficer::getAirline(){return info.airline;}
int CheckInOfficer::getNumber() {return info.number;}

void CheckInOfficer::DoWork(){
	while(info.work){		// While there are still passengers who haven't checked in
		CheckInLock->Acquire();		// Acquire line lock to see if there are passengers in line
		int x = AIRLINE_COUNT*CHECKIN_COUNT + info.airline - 1;		// Check Executive Line for your airline first
		if(CheckInLine[x] > 0){
			CPInfo[x].line = info.number;
			CheckInCV[x]->Signal(CheckInLock);
		}else if (CheckInLine[info.number] > 0){		// If no executive, then check your normal line
			CheckInCV[info.number]->Signal(CheckInLock);
		}else {		// Else, there are no passengers waiting and you can go on break
			setBreak();
			CheckInBreakCV[info.number]->Wait(CheckInLock);		// Go to sleep until manager wakes you up
			continue;		// When woken up, restart from top of while loop
		}
		CheckInLocks[info.number]->Acquire();
		CheckInLock->Release();
		CheckInOfficerCV[info.number]->Wait(CheckInLocks[info.number]);		// Wait for passenger to give you baggage and airline
		 
		info.bags = CPInfo[info.number].bag;		// Place baggage info in shared struct into internal struct
		CPInfo[info.number].bag.clear();		// Clear the shared struct so next passenger wont overwrite baggage
		for (int i = 0; i < (signed)info.bags.size(); i++){		
			info.bags[i].airlineCode = info.airline;		// Add airline code to baggage
			BaggageLock->Acquire();			// Acquire lock for putting baggage on conveyor
			conveyor.push_back(info.bags[i]);		// Place baggage onto conveyor belt for Cargo Handlers
			BaggageLock->Release();		// Release baggage lock
		}
		airlineSeatLock->Acquire();
		int seat = -1;		// Default plane seat is -1
		for (int i = (AIRLINE_COUNT - 1)*50; i < AIRLINE_COUNT * 50; i++ ){		// Airline 1 is 0-49, Airline 2 is 50-99, Airline 3 is 100-149
			if (seats[i]){		// First available seat index is used
				seats[i] = false;
				seat = i;
			}
		}
		if (seat == -1){		// No Spots are left on the airline
			std::cout<<"No spots left on airline!\n";
			return;
		}
		airlineSeatLock->Release();
		CPInfo[info.number].seat = seat;		// Tell Passenger Seat number
		CheckInOfficerCV[info.number]->Signal(CheckInLocks[info.number]);
		CheckInOfficerCV[info.number]->Wait(CheckInLocks[info.number]);		// Passenger will wake up you when they leave, starting the loop over again
	}
}

//----------------------------------------------------------------------
// Cargo Handler
//----------------------------------------------------------------------
CargoHandler::CargoHandler(int n){
	name = n;
}

CargoHandler::~CargoHandler(){}

void CargoHandler::DoWork(){
	cout << "Cargo Handler " << name << " returned from break" << endl;
	onBreak = false;
	while(!conveyor.empty()){
		// grab the first bag on the conveyor belt and move it to the plane
		CargoHandlerLock->Acquire();
		//cout << conveyor.size() << endl;
		Baggage temp = conveyor[0];
		conveyor.pop_front();
		CargoHandlerLock->Release();
		aircraftBaggageCount[temp.airlineCode]++;
		aircraftBaggageWeight[temp.airlineCode] += temp.weight;
		cout << "Cargo Handler " << name << " picked bag of airline " << temp.airlineCode << " with weighing " << temp.weight << " lbs" << endl;
	}
	
	if(conveyor.empty()){
		onBreak = true;
		//if DoWork is called and the conveyor is empty, nap time
		CargoHandlerLock->Acquire();
		cout << "Cargo Handler " << name << " is going for a break" << endl;
		//wait for the manager to ruin your fun
		CargoHandlerCV->Wait(CargoHandlerLock);
		CargoHandlerLock->Release();
	}
}

//----------------------------------------------------------------------
// Screening Officer
//----------------------------------------------------------------------
ScreeningOfficer::ScreeningOfficer(int i){
	number = i;
}
ScreeningOfficer::~ScreeningOfficer(){}

void ScreeningOfficer::DoWork(){
	while(true){
		ScreenLines -> Acquire();
		if (ScreenLine[0] > 0){
			SPInfo[SCREEN_COUNT+1].line = number;
			ScreenLineCV[0]->Signal(ScreenLines);
		}
		ScreenLocks[number]->Acquire();
		ScreenLines->Release();
		ScreenOfficerCV[number]->Wait(ScreenLocks[number]);		// Wait for Passenger to start conversation
		
		int x = rand() % 5;		// Generate random value for pass/fail
		bool pass_fail = true;		// Default is pass
		if (x == 0) pass_fail = false;		// 20% of failure
		
		SecurityAvail->Acquire();
		for (int i = 0; i < SCREEN_COUNT; i++){
			bool y = Security[i]->getAvail();
			if (y){
				SSInfo[i].pass = pass_fail;
				SSInfo[i].ScreenLine = number;
				SPInfo[number].line = i;
			}
		}
		x = SPInfo[number].line;
		SecurityLocks[x]->Acquire();
		SecurityAvail->Release();
		SecurityOfficerCV[x]->Signal(SecurityLocks[x]);
		SecurityOfficerCV[x]->Wait(SecurityLocks[x]);
		ScreenOfficerCV[number]->Signal(ScreenLocks[number]);
		ScreenOfficerCV[number]->Wait(ScreenLocks[number]);
	}
}

//----------------------------------------------------------------------
// Security Officer
//----------------------------------------------------------------------
SecurityOfficer::SecurityOfficer(int i){
	number = i;
}
SecurityOfficer::~SecurityOfficer(){}

void SecurityOfficer::DoWork(){
	while(true){
		SecurityAvail->Acquire();
		if (!available){
			available = true;
		}
		SecurityLocks[number]->Acquire();
		SecurityAvail->Release();
		SecurityOfficerCV[number]->Wait(SecurityLocks[number]);
		
		pass_fail = SSInfo[number].pass;
		SecurityAvail->Acquire();
		available = false;
		SecurityAvail->Release();
		SecurityOfficerCV[number]->Signal(SecurityLocks[number]);
		SecurityOfficerCV[number]->Wait(SecurityLocks[number]);
		
		int x = rand() % 5;		// Generate random value for pass/fail
		SecurityPass = true;		// Default is pass
		if (x == 0) SecurityPass = false;		// 20% of failure
		
		TotalPass = true;
		if (!pass_fail || !SecurityPass) TotalPass = false;
		
		// If False tell Passenger to go questioning, if true, then tell passenger to move forward
		
	}
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

// Test A1
// Passenger & Airport Liaison Interaction
void testPassenger(int passengerIndex) {
	Passenger *p = new Passenger(passengerIndex);
	p->ChooseLiaisonLine();
}

void testCH(int CHIndex){
	CargoHandler *c = new CargoHandler(CHIndex);
	c->DoWork();
	cout << "Did work " << CHIndex << endl;
}
	
void testLiaison(int liaisonIndex) {
	printf("Creating Liaison Officer %d\n", liaisonIndex);
	liaisonLine[liaisonIndex] = 0;
	char* name = "Liaison Line CV " + liaisonIndex;
	Condition *tempCondition = new Condition(name);
	liaisonLineCV[liaisonIndex] = tempCondition;
	char* name4 = "Liaison Officer CV " + liaisonIndex;
	tempCondition = new Condition(name4);
	liaisonOfficerCV[liaisonIndex] = tempCondition;
	char* name2 = "Liaison Line Lock " + liaisonIndex;
	Lock *tempLock = new Lock(name2);
	liaisonLineLocks[liaisonIndex] = tempLock;
	char* name3 = "Liaison Officer " + liaisonIndex;
	LiaisonOfficer *tempLiaison = new LiaisonOfficer(liaisonIndex);
	liaisonOfficers[liaisonIndex] = tempLiaison;
	
	liaisonOfficers[liaisonIndex]->DoWork();
}

void AirportTests() {
	printf("================\n");
	printf("TESTING PART 2\n");
	printf("================\n");
	
	Thread *t;
	
	for(int i = 0; i < 23; i++){
		conveyor.push_back(Baggage());
		conveyor[i].airlineCode = i % 3;
		conveyor[i].weight = rand() % 31 + BAGGAGE_WEIGHT;
		//cout << conveyor[i].airlineCode << " " << conveyor[i].weight << endl;
	}
	
	for(int i = 0; i < 5; i++) {
		t = new Thread("");
		t->Fork((VoidFunctionPtr)testLiaison,i);
	}
	
	for(int i = 0; i < 5; i++) {
		t = new Thread("");
		t->Fork((VoidFunctionPtr)testPassenger,i);
	}
	
	for(int i = 0; i < 5; i++) {
		t = new Thread("");
		cout << "testCH " << i << endl;
		t->Fork((VoidFunctionPtr)testCH,i);
	}
	cout << "exited for loops" << endl;
}
