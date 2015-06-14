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

using namespace std;
int gates[AIRLINE_COUNT];		// Tracks gate numbers for each airline
int LiaisonSeat[AIRLINE_COUNT];
bool ScreeningResult[PASSENGER_COUNT];
LiaisonOfficer *liaisonOfficers[LIAISONLINE_COUNT];		// Array of Liaison Officers
CheckInOfficer *CheckIn[CHECKIN_COUNT*AIRLINE_COUNT];		// Array of CheckIn Officers
SecurityOfficer *Security[SCREEN_COUNT];		// Array of Security Officers
ScreeningOfficer *Screen[SCREEN_COUNT];		// Array of Screening Officers
vector<CargoHandler*> cargoHandlers;				//Vector of Cargo Handlers
LiaisonPassengerInfo * LPInfo = new LiaisonPassengerInfo[LIAISONLINE_COUNT];		// Array of Structs that contain info from passenger to Liaison
CheckInPassengerInfo * CPInfo = new CheckInPassengerInfo[CHECKIN_COUNT*AIRLINE_COUNT+AIRLINE_COUNT];		// Array of Structs that contain info from pasenger to CheckIn
ScreenPassengerInfo * SPInfo = new ScreenPassengerInfo[SCREEN_COUNT+1];		// Array of Structs that contain info from screening to passenger
SecurityScreenInfo * SSInfo = new SecurityScreenInfo[SCREEN_COUNT];		// Array of structs that contains info from security to screener
SecurityPassengerInfo * SecPInfo = new SecurityPassengerInfo[SCREEN_COUNT];		// Array of structs that contains info from security to passenger

deque<Baggage> conveyor;		// Conveyor queue that takes bags from the CheckIn and is removed by Cargo Handlers
int aircraftBaggageCount[AIRLINE_COUNT];		// Number of baggage on a single airline
int aircraftBaggageWeight[AIRLINE_COUNT];		// Weight of baggage on a single airline
// AirportManager* am = new AirportManager(); //for testing purposes
int boardingLounges[AIRLINE_COUNT];		// Array of count of people waiting in airport lounge for airline to leave
int totalPassengersOfAirline[AIRLINE_COUNT];		// Total passengers that should be on an airline
int totalBaggage[AIRLINE_COUNT];
int totalWeight[AIRLINE_COUNT];

int liaisonBaggageCount[AIRLINE_COUNT];			// baggage count from liaison's perspective, per each airline

int simNumOfPassengers;
int simNumOfAirlines;
int simNumOfLiaisons;
int simNumOfCIOs;
int simNumOfCargoHandlers;
int simNumOfScreeningOfficers;
std::vector<Passenger *> simPassengers;
std::vector<LiaisonOfficer *> simLiaisons;
std::vector<CheckInOfficer *> simCIOs;
std::vector<CargoHandler *> simCargoHandlers;
std::vector<ScreeningOfficer *> simScreeningOfficers;
std::vector<SecurityOfficer *> simSecurityOfficers;
AirportManager *simAirportManager;

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
	// INITIALIZATION OF ARRAYS, CVS, LOCKS, AND MORE
	for (int i = 0; i < AIRLINE_COUNT; i++){
		gates[i] = i;
		boardingLounges[i] = 0;
		totalPassengersOfAirline[i] = AIRLINE_SEAT;
		aircraftBaggageCount[i] = 0;		// Number of baggage on a single airline
		aircraftBaggageWeight[i] = 0;		// Weight of baggage on a single airline
	}
	
	for (int i = 0; i < AIRLINE_COUNT; i++){
		LiaisonSeat[i] = AIRLINE_SEAT;
		Lock *tempLock = new Lock("Gate Locks");
		gateLocks[i] = tempLock;
		Condition *tempCondition = new Condition("Gate Lock CV");
		gateLocksCV[i] = tempCondition;	
	}
	
	for (int i = 0; i < AIRLINE_COUNT*AIRLINE_SEAT; i++){
		ScreeningResult[i] = true;
	}
	
	for (int i = 0; i < AIRLINE_COUNT; i++){
		AirlineBaggage[i] = new Lock("Ariline Baggage Lock");
	}
	
// -----------------------------------[ Setting Up Singular Locks ]--------------------------
	liaisonLineLock = new Lock("Liaison Line Lock");
	CheckInLock = new Lock("CheckIn Line Lock");
	ScreenLines = new Lock("Screen Line Lock");
	airlineSeatLock = new Lock("Airline Seat Lock");
	LiaisonSeats = new Lock("Liaison Seat Lock");
	BaggageLock = new Lock("Baggage Lock");
	SecurityAvail = new Lock("Security Availability lock");
	SecurityLines = new Lock("Security Line Lock");	
// -----------------------------------[ Setting Up Check In Officer Locks and CVs ]--------------------------

	for (int i = 0; i < AIRLINE_COUNT; i++){
		for (int y = 0; y < CHECKIN_COUNT; y++){
			int x = (y + i) + (AIRLINE_COUNT + 1) * i;
			CheckInLine[x] = 0;
			CheckIn[x] = new CheckInOfficer(x);
			CheckInLocks[x] = new Lock("CheckIn Officer Lock");
			CheckInBreakCV[x] = new Condition("CheckIn Break Time CV");
			CheckInCV[x] = new Condition("CheckIn Line CV");
			CheckInOfficerCV[x] = new Condition("CheckIn Officer CV");
		}
	}

	for (int i = 0; i < AIRLINE_COUNT; i++){
		int x = AIRLINE_COUNT * CHECKIN_COUNT + i;
		CheckInLine[x] = 0;
		CheckIn[x] = new CheckInOfficer(x);
		CheckInLocks[x] = new Lock("CheckIn Officer Lock");
		CheckInCV[x] = new Condition("CheckIn Line CV");
	}

// -----------------------------------[ Setting Up Liaison ]--------------------------

	for(int i = 0; i < LIAISONLINE_COUNT; i++) {
		liaisonLine[i] = 0;
		liaisonLineCV[i] = new Condition("Liaison Line CV " + i);
		liaisonOfficerCV[i] = new Condition("Liaison Officer CV " + 1);
		liaisonLineLocks[i] = new Lock("Liaison Line Lock " + i);
		liaisonOfficers[i] = new LiaisonOfficer(i);
		printf("Debug: Created Liaison Officer %d\n", i);
	}

// -----------------------------------[ Setting Up Security and Screening ]--------------------------
	for (int i = 0; i < SCREEN_COUNT; i++){
		SecurityLine[i] = 0;
		SecurityOfficer *tempSecurity = new SecurityOfficer(i);
		Security[i] = tempSecurity;
		ScreeningOfficer *tempScreen = new ScreeningOfficer(i);
		Screen[i] = tempScreen;
		char *name = "Screen Officer CV";
		Condition *tempCondition5 = new Condition(name);
		ScreenOfficerCV[i] = tempCondition5;
		name = "Security Officer CV";
		Condition *tempCondition6 = new Condition(name);
		SecurityOfficerCV[i] = tempCondition6;
		name = "Screen Lock";
		Lock *tempLock3 = new Lock(name);
		ScreenLocks[i] = tempLock3;
		name = "Security Lock";
		Lock *tempLock4 = new Lock(name);
		SecurityLocks[i] = tempLock4;
		Condition *tempCondition8 = new Condition("Security line CV");
		SecurityLineCV[i] = tempCondition8;
	}
	
	for (int i = 0; i < SCREEN_COUNT;i++){
		SecurityAvailability[i] = true;
	}
	
	char *name = "Screen Line CV";
	Condition *tempCondition7 = new Condition(name);
	ScreenLineCV[0] = tempCondition7;	
// -----------------------------------[ Setting Up Baggage Conveyor ]--------------------------

	for (int i = 0; i < AIRLINE_COUNT; i++){
		AirlineBaggage[i] = new Lock("Airline Baggage Lock");
	}
	CargoHandlerLock = new Lock("Cargo Handler Lock");
	CargoHandlerCV = new Condition("Cargo Handler CV ");
	
	for (int i = 0; i < AIRLINE_COUNT*AIRLINE_SEAT; i++){
		seats[i] = true;
	}
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
	if(rand() % 3 == 1){		// 25% of being executive class
		economy = false;
	}
	baggageCount = rand() % 2 + BAGGAGE_COUNT;		// Randomly have people with 3 baggage
	for(int i = 0; i < baggageCount; i++){			// Add baggage and their weight to Passenger Baggage Vector
		bags.push_back(Baggage());
		bags[i].weight = rand() % 31 + BAGGAGE_WEIGHT;		// Baggage will weigh from 30 to 60 lb
	}
	//ChooseLiaisonLine();
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

	printf("Passenger %d chose Liaison %d with a line of length %d\n", name, myLine, liaisonLine[myLine]);		// OFFICIAL OUTPUT STATEMENT
	if(liaisonLine[myLine] > 0){		// If you are waiting, go to sleep until signalled by Liaison Officer
		liaisonLine[myLine] += 1;		// Increment size of line you join
		liaisonLineCV[myLine]->Wait(liaisonLineLock);
	}else {
		liaisonLine[myLine] += 1;		// Increment size of line you join
	}
	liaisonLineLock->Release();		// Release the lock you acquired from waking up
	liaisonLineLocks[myLine]->Acquire(); // New lock needed for liaison interaction
	LPInfo->baggageCount = baggageCount; // Adds baggage Count to shared struct array
	liaisonOfficerCV[myLine]->Signal(liaisonLineLocks[myLine]); // Wakes up Liaison Officer
	liaisonOfficerCV[myLine]->Wait(liaisonLineLocks[myLine]); // Goes to sleep until Liaison finishes assigning airline
	airline = LPInfo->airline;		// Gets airline info from Liaison Officer shared struct
	totalBaggage[airline] += baggageCount;
	for(int i = 0; i < baggageCount; i++){			// Add baggage and their weight to Passenger Baggage Vector
		totalWeight[airline] += bags[i].weight;
	}
	liaisonOfficerCV[myLine]->Signal(liaisonLineLocks[myLine]); // Wakes up Liaison Officer to say I'm leaving
	if (liaisonLine[myLine] > 0) liaisonLine[myLine] = liaisonLine[myLine] - 1; //Passenger left the line
	liaisonLineLocks[myLine]->Release(); // Passenger is now leaving to go to airline checking	

	printf("Passenger %d of Airline %d is directed to the check-in counter\n", name, this->getAirline());		// OFFICIAL OUTPUT STATEMENT
	
	currentThread->Yield();
// ----------------------------------------------------[ Going to CheckIn ]----------------------------------------

	CheckInLock->Acquire();		// Acquire lock to find shortest line
	if(!this->getClass()){		// If executive class
		myLine = CHECKIN_COUNT*AIRLINE_COUNT+airline; // There are always airline count more executive lines than economy lines
		printf("Passenger %d of Airline %d is waiting in the executive class line\n", name, this->getAirline());		// OFFICIAL OUTPUT STATEMENT
	} else { 
		myLine = (airline)*CHECKIN_COUNT;		// Economy lines are in intervals of Check In Count
		for (int i = (airline)*CHECKIN_COUNT; i < airline*CHECKIN_COUNT; i++){ // Find shortest line for my airline
			if (CheckInLine[i] < CheckInLine[myLine]) myLine = i;
		}
		printf("Passenger %d of Airline %d chose Airline Check-In staff %d with a line length %d\n", name, this->getAirline(), myLine, CheckInLine[myLine]);		// OFFICIAL OUTPUT STATEMENT
	}
	
	if (economy && CheckInLine[myLine] == 0){		// If first person in line for economy passages, message the officer (they may be on break)
		CheckInBreakCV[myLine]->Signal(CheckInLock);
	} else if (!economy && CheckInLine[myLine] == 0){		// If first person in line for executive passengers
		for (int i = 0; i < CHECKIN_COUNT; i++){
			if (CheckInLine[i] == 0) CheckInBreakCV[i]->Signal(CheckInLock);	// Message all Officers with no line (they may be on break)
		}
	}
	CheckInLine[myLine] += 1;		// Increment line when I enter line
	CheckInCV[myLine]->Wait(CheckInLock);		// Wait for CheckIn Officer to signal
	int oldLine = myLine;		// Save old line to decrement it later when you leave
	myLine = CPInfo[myLine].line;		// Sets its line to that of the Officer
	CheckInLocks[myLine]->Acquire();
	CheckInLock->Release();
	
	CPInfo[myLine].baggageCount = baggageCount;		// Place baggage onto counter (into shared struct)
	for (int i = 0; i < baggageCount; i++){		// Add baggage info to shared struct for CheckIn Officer
		CPInfo[myLine].bag.push_back(Baggage());
		CPInfo[myLine].bag[i].weight = bags[i].weight;
	}
	CPInfo[myLine].passenger = name;		// Tell Officer passenger name 
	CPInfo[myLine].IsEconomy = economy;		// Tell Officer passenger class
	CheckInOfficerCV[myLine]->Signal(CheckInLocks[myLine]);		// Wake up CheckIn Officer 
	CheckInOfficerCV[myLine]->Wait(CheckInLocks[myLine]);
	seat = CPInfo[myLine].seat;		// Get seat number from shared struct
	gate = CPInfo[myLine].gate;		// Get gate number from shared struct
	if (CheckInLine[oldLine] > 0) CheckInLine[oldLine] -= 1; //Passenger left the line
	CheckInOfficerCV[myLine]->Signal(CheckInLocks[myLine]); // Wakes up CheckIn Officer to say I'm leaving
	printf("Passenger %d of Airline %d was informed to board at gate %d\n", name, this->getAirline(), gate);		//OFFICIAL OUTPUT STATEMENT
	CheckInLocks[myLine]->Release(); // Passenger is now leaving to go to screening
	
	currentThread->Yield();

// ----------------------------------------------------[ Going to Screening ]----------------------------------------

	ScreenLines->Acquire();
	
	bool test = true;
	if (ScreenLine[0] == 0){		// If first person in line for Screening Officer...
		ScreenLine[0] += 1;		// Increment line length....
		ScreenLines->Release();
		while (test){		// Look for a Screening Officer that isn't busy... (and keep going till you find one)
			printf ("PASSENGER SCREENING\n");
			ScreenLines->Acquire();
			for (int i = 0; i < SCREEN_COUNT; i++){
				if(!(Screen[i]->getBusy())){		// This checks if that officer is busy
					test = Screen[i]->getBusy();		
					myLine = i;
				}
			}
			ScreenLines->Release();
			currentThread->Yield();		// Wait a bit before re looping to give officer time to change his busy state
		}
		ScreenLines->Acquire();
	} else {		// If not first person in line, just add yourself to the line
		ScreenLine[0] += 1;
		ScreenLineCV[0]->Wait(ScreenLines);
	}
	if (test){
		for (int i = 0; i < SCREEN_COUNT; i++){
			if(!(Screen[i]->getBusy())){
				test = Screen[i]->getBusy();
				myLine = i;
			}
		}
	}
	
	Screen[myLine]->setBusy();		// Set the officer that you picked to busy 
	ScreenLocks[myLine]->Acquire();
	ScreenLines->Release();		// Release the lock so others can access officer busy states
	printf("Passenger %d gives the hand-luggage to screening officer %d\n", name, myLine);		// OFFICIAL OUTPUT STATEMENT
	SPInfo[myLine].passenger = name;		// Tell Screening Officer your name
	ScreenOfficerCV[myLine]->Signal(ScreenLocks[myLine]);		// Wake them up
	ScreenOfficerCV[myLine]->Wait(ScreenLocks[myLine]);		// Go to sleep
	
	oldLine = myLine;		// Save old line to signal to Screening you are leaving
	myLine = SPInfo[oldLine].SecurityOfficer;		// Receive line of which Security Officer to wait for
	ScreenOfficerCV[oldLine]->Signal(ScreenLocks[oldLine]);		// Tells Screening Officer Passenger is leaving
	if(ScreenLine[0] > 0) ScreenLine[0] -= 1;		// Decrement line size
	ScreenLocks[oldLine]->Release();
	
	currentThread->Yield();
	
// ----------------------------------------------------[ Going to Security ]----------------------------------------

	SecurityLines->Acquire();
	printf("Passenger %d moves to security inspector %d\n", name, myLine);		// OFFICIAL OUTPUT STATEMENT
	if (SecurityLine[myLine] == 0){		// If first person in line for security...
		SecurityLine[myLine] += 1;		// Increase line length...
		SecurityLocks[myLine]->Acquire();
		SecurityLines->Release();
		SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		// Message Officer because he is probably sleeping
	} else {		// If there is already a line just increment the line
		SecurityLine[myLine] += 1;
		SecurityLocks[myLine]->Acquire();
		SecurityLines->Release();
	}
	SecurityOfficerCV[myLine]->Wait(SecurityLocks[myLine]);
	
	SecPInfo[myLine].passenger = name;		// Tell officer passenger name
	SecPInfo[myLine].questioning = false;		// Tell officer that passenger hasn't been told to do extra questioning
	SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		// Tell Security that you have arrived
	SecurityOfficerCV[myLine]->Wait(SecurityLocks[myLine]);

	NotTerrorist = SecPInfo[myLine].PassedSecurity;		// Boolean of whether the passenger has passed all security
	if (NotTerrorist){		// If they did pass all tests, then go to boarding area
		SecurityLines->Acquire();
		SecurityLine[myLine] -= 1;		// Decrement Line Size 
		SecurityLines->Release();
		SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		// Tell Security officer that passenger is going to boarding area
		SecurityLocks[myLine]->Release();
		
	} else {		// If they failed, then go to questioning
		SecurityLine[myLine] -= 1;		// Decrement line size
		SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		// Tell Security office that passenger is going to boarding area
		SecurityLocks[myLine]->Release();		// Go To Questioning
		
		printf("Passenger %d goes for further questioning\n", name);		// OFFICIAL OUTPUT STATEMENT
		int r = rand() % 2 + 1;		// Random length of questioning
		for (int i = 0; i < r; i++){		// Stay for questioning for a random length of time
			currentThread->Yield();
		}

		SecurityLines->Acquire();		// Lock for waiting in Line
		if (SecurityLine[myLine] == 0){		// If there is no line when you return...
			SecurityLine[myLine] += 1;		// Add yourself to line length...
			SecurityLocks[myLine]->Acquire();
			SecurityLines->Release();
			SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		// And signal the officer (they are probably sleeping)
		}else {		// Otherwise just add yourself to the line and wait (Security Officer will not take anymore passengers as you are in queue)
			SecurityLine[myLine] += 1;		// Add yourself to line length
			SecurityLines->Release();
			SecurityLocks[myLine]->Acquire();
			SecurityOfficerCV[myLine]->Wait(SecurityLocks[myLine]);		// Wait on security officer to wake you up
			SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		// Tell Security that you have returned from questioning
		}
		SecurityOfficerCV[myLine]->Wait(SecurityLocks[myLine]);		// Wait on security officer to wake you up
		SecPInfo[myLine].passenger = name;		// Tell Security Officer passenger name again
		SecPInfo[myLine].questioning = true;		// Tell officer passenger already underwent questioning and should auto pass
		printf("Passenger %d comes back to security inspector %d after further examination\n", name, myLine);		// OFFICIAL OUTPUT STATEMENT
		SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		// Tell Security that you have returned from questioning
		SecurityOfficerCV[myLine]->Wait(SecurityLocks[myLine]);
		
		if(SecurityLine[myLine] > 0) SecurityLine[myLine] -= 1;		// Leave the line
		SecurityOfficerCV[myLine]->Signal(SecurityLocks[myLine]);		
		SecurityLocks[myLine]->Release();		// Go To Boarding Area
	}
	
// ----------------------------------------------------[ Going to Gate ]----------------------------------------
	
	gateLocks[airline]->Acquire();		// Lock to control passenger flow to boarding lounge
	printf("Passenger %d of Airline %d reached the gate %d\n", name, airline, gate);		// OFFICIAL OUTPUT STATEMENT
	boardingLounges[airline]++;
	gateLocksCV[airline]->Wait(gateLocks[airline]);		// Airline can only leave when all passengers have arrived
	printf("Passenger %d of Airline %d boarded airline %d\n", name, airline, airline);		// OFFICIAL OUTPUT STATEMENT
	gateLocks[airline]->Release();
}

//----------------------------------------------------------------------
// Liaison Officer
//----------------------------------------------------------------------
LiaisonOfficer::LiaisonOfficer(int i){		// Constructor
	info.passengerCount = 0;
	for (int g = 0; g < AIRLINE_COUNT; g++){
		info.airlineBaggageCount[g] = 0;
	}
	info.number = i;
}
LiaisonOfficer::~LiaisonOfficer(){}
int LiaisonOfficer::getPassengerCount() {return info.passengerCount;} // For manager to get passenger headcount
int LiaisonOfficer::getPassengerBaggageCount(int n) {
	return 0;
} // For manager to get passenger bag count

int LiaisonOfficer::getAirlineBaggageCount(int n){ // For manager to get passenger bag count
	return info.airlineBaggageCount[n];
}

void LiaisonOfficer::DoWork(){
	while(true){		// Always be running, never go on break
		liaisonLineLock->Acquire();		// Acquire lock for lining up in order to see if there is someone waiting in your line
		if (liaisonLine[info.number] > 0){		// Check if passengers are in your line
			liaisonLineCV[info.number]->Signal(liaisonLineLock);		// Signal them if there are
		}
		liaisonLineLocks[info.number]->Acquire();		
		liaisonLineLock->Release();
		liaisonOfficerCV[info.number]->Wait(liaisonLineLocks[info.number]);		// Wait for passenger to give you baggage info
		
		// Passenger has given bag Count info and woken up the Liaison Officer
		info.passengerCount += 1;		// Increment internal passenger counter
		LiaisonSeats->Acquire();
		bool flag = true;
		while (flag){
			info.airline = rand() % AIRLINE_COUNT;		// Generate airline for passenger
			if (LiaisonSeat[info.airline] == 0){
			}else {
				LiaisonSeat[info.airline] -= 1;
				flag = false;
			}
		}
		info.airlineBaggageCount[info.airline] += LPInfo[info.number].baggageCount;
		LiaisonSeats->Release();
		LPInfo[info.number].airline = info.airline;		// Put airline number in shared struct for passenger
		liaisonBaggageCount[info.airline] += LPInfo[info.number].baggageCount;
		liaisonOfficerCV[info.number]->Signal(liaisonLineLocks[info.number]); // Wakes up passenger
		liaisonOfficerCV[info.number]->Wait(liaisonLineLocks[info.number]); // Waits for Passenger to say they are leaving
		printf("Airport Liaison %d directed passenger %d of airline %d\n", info.number, info.passengerCount-1, info.airline);		// OFFICIAL OUTPUT STATEMENT
		liaisonLineLocks[info.number]->Release();
	}
}

//----------------------------------------------------------------------
// Check In Staff
//----------------------------------------------------------------------
CheckInOfficer::CheckInOfficer(int i){
	info.number = i;
	info.airline = i/CHECKIN_COUNT;
	info.passengerCount = 0;		// Passenger Count
	OnBreak = false;		// Controls break time
	info.work = true;
}

CheckInOfficer::~CheckInOfficer(){}
int CheckInOfficer::getAirline(){return info.airline;}
int CheckInOfficer::getNumber() {return info.number;}
bool CheckInOfficer::getOnBreak() {return OnBreak;}
void CheckInOfficer::setOffBreak() {OnBreak = false;}

void CheckInOfficer::DoWork(){
	while(info.work){		// While there are still passengers who haven't checked in
		CheckInLock->Acquire();		// Acquire line lock to see if there are passengers in line
		int x = AIRLINE_COUNT*CHECKIN_COUNT + info.airline;		// Check Executive Line for your airline first
		if (OnBreak) setOffBreak();
		if(CheckInLine[x] > 0){
			CPInfo[x].line = info.number;
			CheckInCV[x]->Signal(CheckInLock);
			printf("Airline check-in staff %d of airline %d serves an executive class passenger and economy class line length = %d\n", info.number, info.airline, CheckInLine[info.number]);		// OFFICIAL OUTPUT STATEMENT
		} else if (CheckInLine[info.number] > 0){		// If no executive, then check your normal line
			CheckInCV[info.number]->Signal(CheckInLock);
			printf("Airline check-in staff %d of airline %d serves an economy class passenger and executive class line length = %d\n", info.number, info.airline, CheckInLine[x]);		// OFFICIAL OUTPUT STATEMENT
		}else {		// Else, there are no passengers waiting and you can go on break
			OnBreak = true;
			CheckInBreakCV[info.number]->Wait(CheckInLock);
			CheckInLock->Release();
			continue;
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
			totalBags.push_back(info.bags[i]);
		}
		printf("Airline check-in staff %d of airline %d dropped bags to the conveyor system\n", info.number, info.airline);		// OFFICIAL OUTPUT STATEMENT
		airlineSeatLock->Acquire();
		int seat = -1;		// Default plane seat is -1
		int c = (AIRLINE_COUNT-1)*AIRLINE_SEAT;
		int d = (AIRLINE_COUNT)*AIRLINE_SEAT;
		for (int i = c; i < d; i++ ){		// Airline 1 is 0-49, Airline 2 is 50-99, Airline 3 is 100-149
			if (seats[i]){		// First available seat index is used
				seat = i;
			}
		}
		if (seat == -1){		// No Spots are left on the airline
			std::cout<<"No spots left on airline!\n";
			return;
		} else {
			seats[seat] = false;
		}
		airlineSeatLock->Release();
		int z = CPInfo[info.number].passenger;
		CPInfo[info.number].seat = seat;		// Tell Passenger Seat number
		CPInfo[info.number].gate = gates[info.airline];		// Tell Passenger Gate Number
		info.passengerCount++;
		if (!CPInfo[info.number].IsEconomy){
			printf("Airline check-in staff %d of airline %d informs executive class passenger %d to board at gate %d\n", info.number, info.airline, z, gates[info.airline]);		// OFFICIAL OUTPUT STATEMENT
		} else {
			printf("Airline check-in staff %d of airline %d informs economy class passenger %d to board at gate %d\n", info.number, info.airline, z, gates[info.airline]);		// OFFICIAL OUTPUT STATEMENT
		}
		CheckInOfficerCV[info.number]->Signal(CheckInLocks[info.number]);
		CheckInOfficerCV[info.number]->Wait(CheckInLocks[info.number]);		// Passenger will wake up you when they leave, starting the loop over again
		CheckInLocks[info.number]->Release();
	}
	printf("Airline check-in staff %d is closing the counter\n", info.number);		// OFFICIAL OUTPUT STATEMENT
}

//----------------------------------------------------------------------
// Cargo Handler
//----------------------------------------------------------------------
CargoHandler::CargoHandler(int n){
	name = n;
	// am->AddCargoHandler(this);
	// cargoHandlers.push_back(this);
	// cout << "BEEPBOOP" << this << endl;
	for(int i = 0; i < AIRLINE_COUNT; i++){
		weight[i] = 0;
		count[i] = 0;
	}
}

CargoHandler::~CargoHandler(){}

void CargoHandler::DoWork(){
	while (true){
		onBreak = false;
		CargoHandlerLock->Acquire();
		Baggage temp;		// Baggage that handler will move off conveyor
		if (!conveyor.empty()){		// If the conveyor belt has baggage
			// printf("About to move baggage\n");
			temp = conveyor[0];
			conveyor.pop_front();		// Remove a piece of baggage
		} else {
			onBreak = true;
			printf("Cargo Handler %d is going for a break\n", name);		// OFFICIAL OUTPUT STATEMENT
			CargoHandlerCV->Wait(CargoHandlerLock);		// Sleep until woken up by manager
			printf("Cargo Handler %d returned from break\n", name);		// OFFICIAL OUTPUT STATEMENT
			continue;		// Restart loop
		}
		AirlineBaggage[temp.airlineCode]->Acquire();		// Acquire lock to put baggage on an airline
		CargoHandlerLock->Release();
		aircraftBaggageCount[temp.airlineCode]++;		// Increase baggage count of airline
		aircraftBaggageWeight[temp.airlineCode] += temp.weight;		// Increase baggage weight of airline
		printf("Cargo Handler %d picked bag of airline %d with weighing %d lbs\n", name, temp.airlineCode, temp.weight);		// OFFICIAL OUTPUT STATEMENT
		weight[temp.airlineCode] += temp.weight;		// Increment total weight of baggage this handler has dealt with
		count[temp.airlineCode] ++;		// Increment total count of baggage this handler has dealt with
		AirlineBaggage[temp.airlineCode]->Release();
	}
}

//----------------------------------------------------------------------
// Airport Manager
//----------------------------------------------------------------------

AirportManager::AirportManager(){
	// cout << "GOLIATH ONLINE" << endl;
	for(int i =0; i < AIRLINE_COUNT; i++){
		CIOTotalCount[i] = 0;
		CIOTotalWeight[i] = 0;
		LiaisonTotalCount[i] = 0;
		CargoHandlerTotalWeight[i] = 0;
		CargoHandlerTotalCount[i] = 0;
	}
	liaisonPassengerCount = 0;
	checkInPassengerCount = 0;
	securityPassengerCount = 0;
}

AirportManager::~AirportManager(){}

void AirportManager::DoWork(){
	while(true){
		//if the conveyor belt is not empty and cargo people are on break, wake them up
		// cout << "cargo: " << cargoHandlers.size() << " conveyor: " << conveyor.size() << endl;
		if(!conveyor.empty() && !cargoHandlers.empty()){
			CargoHandlerLock->Acquire();
			int breakCount = 0;
			for(int i = 0; i < (signed)cargoHandlers.size(); i++){
				if(cargoHandlers[i]->getBreak()){
					breakCount++;
				}
			}
			if(breakCount == (signed)cargoHandlers.size()){
				cout << "Airport manager calls back all the cargo handlers from break" << endl;
				CargoHandlerCV->Broadcast(CargoHandlerLock);
			}
			CargoHandlerLock->Release();
		}
		int planeCount = 0;
		// if all passengers and bags have been processed in an airline, release the kraken (plane)
		// cout << boardingLounges[0] << " " << totalPassengersOfAirline[0] << " " << totalBaggage[0] << " " << aircraftBaggageCount[0] << endl;
		for(int i = 0; i < AIRLINE_COUNT; i++){
			if(boardingLounges[i] == totalPassengersOfAirline[i] && totalBaggage[i] == aircraftBaggageCount[i]){
				gateLocks[i]->Acquire();
				cout << "Airport Manager gives a boarding call to airline " << i << endl;
				gateLocksCV[i]->Broadcast(gateLocks[i]);
				gateLocks[i]->Release();
				planeCount++;
			}
		}
		if(planeCount == AIRLINE_COUNT){
			cout << "END OF DAY" << endl;
			EndOfDay();
			break;
		}
		for (int i = 0; i < 10; i++){
			currentThread->Yield();
		}
	}
}

void AirportManager::EndOfDay(){
	for(int i = 0; i < (signed)cargoHandlers.size(); i++){
		for(int j = 0; j < AIRLINE_COUNT; j++){
			CargoHandlerTotalWeight[j] += cargoHandlers[i]->getWeight(j);
			CargoHandlerTotalCount[j] += cargoHandlers[i]->getCount(j);
		}
	}
	for(int i = 0; i < CHECKIN_COUNT; i++){
		for(int j = 0; j < (signed)CheckIn[i]->totalBags.size(); j++){
			CIOTotalWeight[CheckIn[i]->totalBags[j].airlineCode] += CheckIn[i]->totalBags[j].weight;
		}
		checkInPassengerCount += CheckIn[i]->getPassengerCount();
	}

	for(int i = 0; i < LIAISONLINE_COUNT; i++){
		for(int j = 0; j < AIRLINE_COUNT; j++){
			LiaisonTotalCount[j] += liaisonOfficers[i]->getAirlineBaggageCount(j);
		}
		liaisonPassengerCount += liaisonOfficers[i]->getPassengerCount();
	}
	
	for(int i = 0; i < SCREEN_COUNT; i++){
		securityPassengerCount += Security[i]->getPassengers();
	}
	for(int i = 0; i < AIRLINE_COUNT; i++){
		cout << "Passenger count reported by airport liaison = " << liaisonPassengerCount << endl;
		cout << "Passenger count reported by airline check-in staff = " << checkInPassengerCount << endl;
		cout << "Passenger count reported by security inspector = " << securityPassengerCount << endl;
		cout << "From setup: Baggage count of airline " << i << " = " << totalBaggage[i] << endl; //OFFICIAL
		cout << "From airport liaison: Baggage count of airline " << i << " = " << LiaisonTotalCount[i] << endl; //OFFICIAL
		cout << "From cargo handlers: Baggage count of airline " << i << " = " << CargoHandlerTotalCount[i] << endl; //OFFICIAL
		cout << "From setup: Baggage weight of airline " << i << " = " << totalWeight[i] << endl; // OFFICIAL
		cout << "From airline check-in staff: Baggage weight of airline " << i << " = " << CIOTotalWeight[i] << endl; //OFFICIAL
		cout << "From cargo handlers: Baggage weight of airline " << i << " = " << CargoHandlerTotalWeight[i] << endl; //OFFICIAL
	}
}

//----------------------------------------------------------------------
// Screening Officer
//----------------------------------------------------------------------
ScreeningOfficer::ScreeningOfficer(int i){
	number = i;
	ScreenLines -> Acquire();
	IsBusy = false;		// Set Officer to available
	ScreenLines -> Release();
}
ScreeningOfficer::~ScreeningOfficer(){}

void ScreeningOfficer::DoWork(){
	while(true){
		ScreenLines -> Acquire();
		if (IsBusy) IsBusy = false;		// If busy, should no longer be busy 
		if (ScreenLine[0] > 0){		// Checks if the screening line has passengers
			ScreenLineCV[0]->Signal(ScreenLines);		// Wake them if there are
		}
		ScreenLocks[number]->Acquire();
		ScreenLines->Release();
		ScreenOfficerCV[number]->Wait(ScreenLocks[number]);		// Wait for Passenger to start conversation

		int z = SPInfo[number].passenger;		// Find passenger name
		int x = rand() % 5;		// Generate random value for pass/fail
		ScreenPass = true;		// Default is pass
		if (x == 0) ScreenPass = false;		// 20% of failure
		ScreeningResult[z] = ScreenPass;
		if (ScreenPass){		// If passenger passed test
			printf("Screening officer %d is not suspicious of the hand luggage of passenger %d\n", number, z);		// OFFICIAL OUTPUT STATEMENT
		}else {
			printf("Screening officer %d is suspicious of the hand luggage of passenger %d\n", number, z);		// OFFICIAL OUTPUT STATEMENT
		}
		
		bool y = false;
		while(!y){		// Wait for Security Officer to become available
			for (int i = 0; i < SCREEN_COUNT; i++){		// Iterate through all security officers
				SecurityAvail->Acquire();
				y = SecurityAvailability[i];		// See if they are busy
				if (y){			// If a security officer is not busy, obtain his number and inform passenger
					SPInfo[number].SecurityOfficer = i;
					SecurityAvailability[i] = false;
					break;
				}
				SecurityAvail->Release();
			}
			for (int i = 0; i < 10; i++){		// Wait for a while so Officer can change availability status
				currentThread->Yield();
				currentThread->Yield();
				currentThread->Yield();
			}
		}
		
		printf("Screening officer %d directs passenger %d to security inspector %d\n", number, z, SPInfo[number].SecurityOfficer);		// OFFICIAL OUTPUT STATEMENT
		ScreenOfficerCV[number]->Signal(ScreenLocks[number]);		// Signal Passenger that they should move on
		ScreenOfficerCV[number]->Wait(ScreenLocks[number]);
		ScreenLocks[number]->Release();
	}
}

//----------------------------------------------------------------------
// Security Officer
//----------------------------------------------------------------------
SecurityOfficer::SecurityOfficer(int i){
	number = i;
	SecurityAvail -> Acquire();
	SecurityAvailability[i] = true;
	SecurityAvail -> Release();
	PassedPassengers = 0;
}
SecurityOfficer::~SecurityOfficer(){}

void SecurityOfficer::DoWork(){
	while(true){
		SecurityLines->Acquire();
		SecurityLocks[number]->Acquire();
		if (SecurityLine[number] > 0){		// Always see if Officer has a line of returning passengers from questioning
			SecurityLineCV[number]->Signal(SecurityLocks[number]);
		} else {
			//SecurityAvail->Acquire();
			SecurityAvailability[number] = true;		// Set itself to available
			//SecurityAvail->Release();
		}
		SecurityLines->Release();
		SecurityOfficerCV[number]->Wait(SecurityLocks[number]);
		SecurityOfficerCV[number]->Signal(SecurityLocks[number]);
		SecurityOfficerCV[number]->Wait(SecurityLocks[number]);
		int z = SecPInfo[number].passenger;		// Get passenger name from passenger
		didPassScreening = ScreeningResult[z];		// get result of passenger screening test from screening officer
		
		// Passenger will wake up Security Officer
		int x = rand() % 5;		// Generate random value for pass/fail
		SecurityPass = true;		// Default is pass
		if (x == 0) SecurityPass = false;		// 20% of failure
		
		if (SecPInfo[number].questioning){		// Passenger has just returned from questioning
			TotalPass = true;		// Allow returned passenger to contiarea
			PassedPassengers += 1;
			SecurityOfficerCV[number]->Signal(SecurityLocks[number]);		// Signal passenger to move onwards
			printf("Security inspector %d permits returning passenger %d to board\n", number, z);		// OFFICIAL OUTPUT STATEMENT
			SecurityOfficerCV[number]->Wait(SecurityLocks[number]);
		}else {		// Passenger is first time
			if (!didPassScreening || !SecurityPass){
				TotalPass = false;		// Passenger only passes if they pass both tests
				printf("Security inspector %d is suspicious of the passenger %d\n", number, z);		// OFFICIAL OUTPUT STATEMENT
			}else {
				TotalPass = true;
				printf("Security inspector %d is not suspicious of the passenger %d\n", number, z);		// OFFICIAL OUTPUT STATEMENT
			}
			SecPInfo[number].PassedSecurity = TotalPass;
		
			if(!TotalPass){		// If they don't pass, passenger will go for questioning and the Officer is free again
				SecurityOfficerCV[number]->Signal(SecurityLocks[number]);		// Signal passenger to move to questioning
				printf("Security inspector %d asks passenger %d to go for further examination\n",number, z);		// OFFICIAL OUTPUT STATEMENT
				SecurityOfficerCV[number]->Wait(SecurityLocks[number]);
			}else{		// If they pass, tell passenger to go to boarding area and increment passed passenger count
				PassedPassengers += 1;
				printf("Security inspector %d allows passenger %d to board\n", number, z);		// OFFICIAL OUTPUT STATEMENT
				SecurityOfficerCV[number]->Signal(SecurityLocks[number]);		// Signal passenger to move onwards
				SecurityOfficerCV[number]->Wait(SecurityLocks[number]);
			}
		}
		SecurityLocks[number]->Release();
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


void setupAirlines(int airlineCount) {
	for (int i = 0; i < airlineCount; i++){
		gates[i] = i;
		boardingLounges[i] = 0;
		totalPassengersOfAirline[i] = 8;
		aircraftBaggageCount[i] = 0;		// Number of baggage on a single airline
		aircraftBaggageWeight[i] = 0;		// Weight of baggage on a single airline
	}
	
	for (int i = 0; i < airlineCount; i++){
		LiaisonSeat[i] = AIRLINE_SEAT;
		Lock *tempLock = new Lock("Gate Locks");
		gateLocks[i] = tempLock;
		Condition *tempCondition = new Condition("Gate Lock CV");
		gateLocksCV[i] = tempCondition;	
	}
	
	for (int i = 0; i < airlineCount * AIRLINE_SEAT; i++){
		ScreeningResult[i] = true;
	}
}

void createPassengers(int quantity) {
	for(int i = 0; i < quantity; i++) {
		simPassengers.push_back(new Passenger(i));
		printf("Debug: Created Passenger %d\n", i);
	}
}

void createLiaisons(int quantity) {
	for(int i = 0; i < quantity; i++) {
		liaisonLine[i] = 0;
		liaisonLineCV[i] = new Condition("Liaison Line CV " + i);
		liaisonOfficerCV[i] = new Condition("Liaison Officer CV " + 1);
		liaisonLineLocks[i] = new Lock("Liaison Line Lock " + i);
		liaisonOfficers[i] = new LiaisonOfficer(i);
		simLiaisons.push_back(liaisonOfficers[i]);
		printf("Debug: Created Liaison Officer %d\n", i);
	}
}

void setupEconomyCIOs(int airlineCount, int quantity) {
	for (int i = 0; i < airlineCount; i++){
		for (int y = 0; y < quantity; y++){
			int x = (y + i) + (airlineCount + 1) * i;
			CheckInLine[x] = 0;
			CheckIn[x] = new CheckInOfficer(x);
			CheckInLocks[x] = new Lock("CheckIn Officer Lock");
			CheckInBreakCV[x] = new Condition("CheckIn Break Time CV");
			CheckInCV[x] = new Condition("CheckIn Line CV");
			CheckInOfficerCV[x] = new Condition("CheckIn Officer CV");
		}
	}
}

void setupExecutiveCIOs(int airlineCount, int quantity) {
	for (int i = 0; i < airlineCount; i++){
		int x = airlineCount * quantity + i;
		CheckInLine[x] = 0;
		CheckIn[x] = new CheckInOfficer(x);
		CheckInLocks[x] = new Lock("CheckIn Officer Lock");
		CheckInCV[x] = new Condition("CheckIn Line CV");
	}
}

void createCIOs(int airlineCount, int quantity) {
	for(int i = 0; i < airlineCount; i++) {
		for(int j = 0; j < quantity; j++) {
			int x = j + i + (airlineCount + 1) * i;
			simCIOs.push_back(new CheckInOfficer(x));
			CheckIn[j] = simCIOs.back();
			printf("Debug: Created Check In Officer %d\n", x);
		}
	}
}

void createAirportManager() {
	simAirportManager = new AirportManager();
}

void createSecurityAndScreen(int quantity) {
	for (int i = 0; i < quantity; i++){
		SecurityLine[i] = 0;
		Security[i] = new SecurityOfficer(i);
		Screen[i] = new ScreeningOfficer(i);
		ScreenOfficerCV[i] = new Condition("Screen Officer CV");
		SecurityOfficerCV[i] = new Condition("Security Officer CV");
		ScreenLocks[i] = new Lock("Screen Lock");
		SecurityLocks[i] = new Lock("Security Lock");
		SecurityLineCV[i] = new Condition("Security line CV");
		simScreeningOfficers.push_back(Screen[i]);
		simSecurityOfficers.push_back(Security[i]);
	}
	
	for (int i = 0; i < quantity;i++){
		SecurityAvailability[i] = true;
	}
	
	ScreenLineCV[0] = new Condition("Screen Line CV");
}

void createCargoHandlers(int quantity) {
	for(int i = 0; i < quantity; i++) {
		CargoHandler *c = new CargoHandler(i);
		simCargoHandlers.push_back(c);
		printf("Debug: Created Cargo Handler %d\n", i);
	}
	cargoHandlers = simCargoHandlers;
}

void testPassenger(int i) {
	simPassengers.at(i)->ChooseLiaisonLine();
}

void testLiaison(int i){
	simLiaisons.at(i)->DoWork();
}

void testCheckIn(int i){
	simCIOs.at(i)->DoWork();
}

void testCargo(int i){
	simCargoHandlers.at(i)->DoWork();
}

void testAirportManager() {
	simAirportManager->DoWork();
}

void setupBaggageAndCargo(int airlineCount) {
	for (int i = 0; i < airlineCount; i++){
		AirlineBaggage[i] = new Lock("Airline Baggage Lock");
	}
	CargoHandlerLock = new Lock("Cargo Handler Lock");
	CargoHandlerCV = new Condition("Cargo Handler CV ");
	
	for (int i = 0; i < airlineCount * AIRLINE_SEAT; i++){
		seats[i] = true;
	}
}

void testScreen(int i){
	simScreeningOfficers.at(i)->DoWork();
}

void testSecurity(int i){
	simSecurityOfficers.at(i)->DoWork();
}

void setupSingularLocks() {
	liaisonLineLock = new Lock("Liaison Line Lock");
	CheckInLock = new Lock("CheckIn Line Lock");
	ScreenLines = new Lock("Screen Line Lock");
	airlineSeatLock = new Lock("Airline Seat Lock");
	LiaisonSeats = new Lock("Liaison Seat Lock");
	BaggageLock = new Lock("Baggage Lock");
	SecurityAvail = new Lock("Security Availability lock");
	SecurityLines = new Lock("Security Line Lock");
}

void setup(){
	srand (time(NULL));
	
	createPassengers(8);
	// createAirportManager();
	setupAirlines(AIRLINE_COUNT);
	setupSingularLocks();
	setupEconomyCIOs(AIRLINE_COUNT, CHECKIN_COUNT);
	setupExecutiveCIOs(AIRLINE_COUNT, CHECKIN_COUNT);
	createCIOs(AIRLINE_COUNT, CHECKIN_COUNT);
	createLiaisons(LIAISONLINE_COUNT);
	createSecurityAndScreen(SCREEN_COUNT);
	setupBaggageAndCargo(AIRLINE_COUNT);
	createCargoHandlers(6);
}

void RunSim() {
	printf("Enter the number of passengers: ");
	std::cin >> simNumOfPassengers;
	printf("Enter the number of airlines: ");
	std::cin >> simNumOfAirlines;
	printf("Enter the number of liaisons: ");
	std::cin >> simNumOfLiaisons;
	printf("Enter the number of check in staff: ");
	std::cin >> simNumOfCIOs;
	printf("Enter the number of cargo handlers: ");
	std::cin >> simNumOfCargoHandlers;
	printf("Enter the number of screening officers: ");
	std::cin >> simNumOfScreeningOfficers;
	printf("\n");
	
	Thread *t;
	
	setupAirlines(simNumOfAirlines);
	setupSingularLocks();
	createAirportManager();
	createPassengers(simNumOfPassengers);
	createLiaisons(simNumOfLiaisons);
	setupEconomyCIOs(simNumOfAirlines, simNumOfCIOs);
	setupExecutiveCIOs(simNumOfAirlines, simNumOfCIOs);
	createCIOs(simNumOfAirlines, simNumOfCIOs);
	createSecurityAndScreen(simNumOfScreeningOfficers);
	setupBaggageAndCargo(simNumOfAirlines);
	createCargoHandlers(simNumOfCargoHandlers);
	
	printf("Setup print statements:\n");
	printf("Number of airport liaisons = %d\n", simNumOfLiaisons);
	printf("Number of airlines = %d\n", simNumOfAirlines);
	printf("Number of check-in staff = %d\n", simNumOfCIOs);
	printf("Number of cargo handlers = %d\n", simNumOfCargoHandlers);
	printf("Number of screening officers = %d\n", simNumOfScreeningOfficers);
	printf("Number of passengers = %d\n", simNumOfPassengers);
	for(int i = 0; i < simNumOfPassengers; i++) {
		printf("Passenger %d: Number of bags = %d\n", i, simPassengers.at(i)->getBaggageCount());
		printf("Passenger %d: Weight of bags = ", i);
		int numberOfBags = simPassengers.at(i)->getBags().size();
		for(int j = 0; j < numberOfBags; j++) {
			printf("%d", simPassengers.at(i)->getBags().at(j).weight);
			if(j < numberOfBags - 1) printf(", ");
			else printf("\n");
		}
	}
}

void AirportTests() {
	printf("================\n");
	printf("TESTING PART 2\n");
	printf("================\n");
	
	for(int i = 0; i < AIRLINE_COUNT; ++i) {
		liaisonBaggageCount[i] = 0;
	}
	
	setup();	// Sets up CVs and Locks
	Thread *t;

	createAirportManager();
	
	t = new Thread("Airport Manager");
	t->Fork((VoidFunctionPtr)testAirportManager, 0);
	
	for(int i = 0; i < AIRLINE_COUNT; i++) {
		for(int j = 0; j < CHECKIN_COUNT; j++) {
			t = new Thread("CheckIn Officer");
			t->Fork((VoidFunctionPtr)testCheckIn, j + i + (AIRLINE_COUNT+1)*i);
		}
	}
	
	for(int i = 0; i < LIAISONLINE_COUNT; i++) {
		t = new Thread("Liaison Officer");
		t->Fork((VoidFunctionPtr)testLiaison, i);
	}
	
	for(int i = 0; i < SCREEN_COUNT; i++) {
		t = new Thread("Screening Officer");
		t->Fork((VoidFunctionPtr)testScreen, i);
		t = new Thread("Security Officer");
		t->Fork((VoidFunctionPtr)testSecurity, i);
	}
	
	for(int i = 0; i < 6; i++) {
		t = new Thread("Cargo Handler");
		t->Fork((VoidFunctionPtr)testCargo, i);
	}
	
	for(int i = 0; i < 8; i++) {
		t = new Thread("Passenger");
		t->Fork((VoidFunctionPtr)testPassenger, i);
	}
}
