/* threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions. */

#include "syscall.h"

#define true 1
#define false 0

/* Max agent consts */
#define PASSENGER_COUNT			9
#define MAX_AIRLINES			3
#define MAX_LIAISONS			3
#define MAX_CIOS				3
#define MAX_CARGOHANDLERS		4
#define MAX_SCREEN				3
#define MAX_BAGS				3
#define AIRLINE_SEAT 			3

int Passenger_ID = 0;
int Liaison_ID = 0;
int Screening_ID = 0;
int CheckIn_ID = 0;
int Security_ID = 0;
int Cargo_ID = 0;

int Passenger_ID_Lock;
int Liaison_ID_Lock;
int CheckIn_ID_Lock;
int Screening_ID_Lock;
int Security_ID_Lock;
int Cargo_ID_Lock;

/*----------------------------------------------------------------------
// Arrays, Lists, and Vectors
//---------------------------------------------------------------------- */
int SecurityAvailability[MAX_SCREEN];		/* Array of Bools for availability of each security officer */
int liaisonLine[MAX_LIAISONS];		/* Array of line sizes for each Liaison Officer */
int CheckInLine[MAX_CIOS*MAX_AIRLINES+MAX_AIRLINES];		/* Array of line sizes for each CheckIn Officer */
int SecurityLine[MAX_SCREEN];		/* Array of line sizes for return passengers from security questioning */
int ScreenLine[MAX_SCREEN];		/* Array of line sizes for each Screening Officer */
int ScreenLineCV[MAX_SCREEN];			/* Condition Variables for the Screening Line */
int ScreenOfficerCV[MAX_SCREEN];		/* Condition Variables for each Screening Officer */
int SecurityOfficerCV[MAX_SCREEN];		/* Condition Variables for each Security Officer */
int SecurityLineCV[MAX_SCREEN];		/* Condition Variables for returning passengers from questioning */
int liaisonLineCV[MAX_LIAISONS];		/* Condition Variables for each Liaison Line */
int liaisonOfficerCV[MAX_LIAISONS];		/* Condition Variables for each Liaison Officer */
int CheckInCV[MAX_CIOS*MAX_AIRLINES+MAX_AIRLINES];		/* Condition Variables for each CheckIn Line */
int CheckInOfficerCV[MAX_CIOS*MAX_AIRLINES];		/* Condition Variables for each CheckIn Officer */
int CheckInBreakCV[MAX_CIOS*MAX_AIRLINES];		/* Condition Variables for each CheckIn Officer Break Time */
int liaisonLineLock;		/* Lock to get into a liaison Line */
int liaisonLineLocks[MAX_LIAISONS];		/* Array of Locks for Liaison Officers */
int CheckInLocks[MAX_CIOS*MAX_LIAISONS];		/* Array of Locks for CheckIn Officers */
int ScreenLocks[MAX_SCREEN];		/* Array of Locks for Screening Officers */
int SecurityLocks[MAX_SCREEN];		/* Array of Locks for Security Officers */
int AirlineBaggage[MAX_LIAISONS]; 		/* Array of Locks for placing baggage on airlines */
int CheckInLock;		/* Lock to get into CheckIn Line */
int ScreenLines;		/* Lock to get into Screening Line */
int CargoHandlerLock;		/* Lock for Cargo Handlers for taking baggage off conveyor */
int CargoHandlerCV;		/* Condition Variable for Cargo Handlers */
int airlineSeatLock;		/* Lock for find seat number for customers */
int BaggageLock;		/* Lock for placing Baggage onto the conveyor */
int SecurityAvail;		/* Lock for seeing if a Security Officer is busy */
int SecurityLines;			/* Lock for returning passengers from Security */
int gateLocks[MAX_AIRLINES];				/* Locks for waiting at the gate */
int gateLocksCV[MAX_AIRLINES];		/* CVs for waiting at the gate */
int seatLock;			/* Lock for assigned airline seats in the Liaison */
int execLineLocks[MAX_AIRLINES];
int execLineCV[MAX_AIRLINES];
/*----------------------------------------------------------------------
// Structs
//---------------------------------------------------------------------- */
typedef struct {
	int weight;
	int airline;
}Baggage_t;

typedef struct{
	int NotTerrorist;
	int gate;
	int name;        
	int seat;	
	int airline;		
	int economy;	
	int myLine;		
	int baggageCount;	
	Baggage_t bags[2];
}Passenger_t;

typedef struct{
	int number;
	int passengerCount;
	Baggage_t bags[2];
	Baggage_t totalBags[PASSENGER_COUNT*2];
	int airline;
	int OnBreak;		/* If there are no passengers in line, the officer goes on break until manager makes them up */
	int work;		/* Always working until all passengers have finished checking in */
}CheckInOfficer_t;

typedef struct{
	int airline;		/* Airline the liaison will assign to the passenger */
	int number;		/* Number of the liaison (which line they control) */
	int passengerCount;		/* Number of passengers the liaison has helped */
	int airlineBaggageCount[MAX_AIRLINES];		/* Array keeping track of baggage count for each passenger */
} LiaisonOfficer_t;

typedef struct{
	int IsBusy;		/* bool controlling whether the screening officer is busy or not */
	int ScreenPass;		/* If the current Passenger Passed Screening, Given to Security Officer */
	int number;
} ScreeningOfficer_t;

typedef struct {
	int PassedPassengers;		/* Number of passengers that passed security */
	int didPassScreening;		/* If the current passenger passed screening */
	int SecurityPass;			/* If the current passenger passed security */
	int TotalPass;				/* If the current passenger passed both screening and security */
	int number;
}SecurityOfficer_t;

typedef struct{
	int name;
	int onBreak;
	int weight[MAX_AIRLINES];
	int count[MAX_AIRLINES];
} CargoHandler_t;

typedef struct{
	int CargoHandlerTotalWeight[MAX_AIRLINES];
	int CargoHandlerTotalCount[MAX_AIRLINES];
	int CIOTotalCount[MAX_AIRLINES];
	int CIOTotalWeight[MAX_AIRLINES];
	int LiaisonTotalCount[MAX_AIRLINES];
	int liaisonPassengerCount;
	int checkInPassengerCount;
	int securityPassengerCount;
} AirportManager_t;

typedef struct{		/* Information passed between Liaison Officer and Passengers */
	int baggageCount;
	int airline;
	int passengerName;
} LiaisonPassengerInfo_t;

typedef struct {		/* Information passed between CheckIn Officer and Passenger */
	int baggageCount;
	int passenger;
	int IsEconomy;
	int seat;
	int gate;
	int line;
	Baggage_t bag[2];
}CheckInPassengerInfo_t;

typedef struct{		/* Information passed between Screening Officer and Passenger */
	int passenger;
	int SecurityOfficer;
} ScreenPassengerInfo_t;

typedef struct{		/* Information passed between Security Officer and Screening Officer */
	int ScreenLine;
} SecurityScreenInfo_t;

typedef struct {		/* Information passed between Security and Passenger */
	int PassedSecurity;
	int questioning;
	int passenger;
}SecurityPassengerInfo_t;

int gates[3];		/* Tracks gate numbers for each airline*/
int ScreeningResult[PASSENGER_COUNT];

LiaisonOfficer_t liaisonOfficers[MAX_LIAISONS];		/* Array of Liaison Officers*/
CheckInOfficer_t CheckIn[MAX_CIOS*MAX_AIRLINES];		/* Array of CheckIn Officers*/
SecurityOfficer_t Security[MAX_SCREEN];		/* Array of Security Officers*/
ScreeningOfficer_t Screen[MAX_SCREEN];		/* Array of Screening Officers*/
CargoHandler_t cargoHandlers[MAX_CARGOHANDLERS];				/* Array of Cargo Handlers*/
LiaisonPassengerInfo_t LPInfo[MAX_LIAISONS];		/* Array of Structs that contain info from passenger to Liaison*/
CheckInPassengerInfo_t CPInfo[MAX_CIOS*MAX_AIRLINES];		/* Array of Structs that contain info from pasenger to CheckIn*/
ScreenPassengerInfo_t SPInfo[MAX_SCREEN+1];		/* Array of Structs that contain info from screening to passenger*/
SecurityScreenInfo_t SSInfo[MAX_SCREEN];		/* Array of structs that contains info from security to screener */
SecurityPassengerInfo_t SecPInfo[MAX_SCREEN];		/* Array of structs that contains info from security to passenger */

int baggageShield[100];
Baggage_t conveyor[PASSENGER_COUNT*2];		/* Conveyor queue that takes bags from the CheckIn and is removed by Cargo Handlers*/
int baggageShiled2[100];
int aircraftBaggageCount[MAX_AIRLINES];		/* Number of baggage on a single airline */
int aircraftBaggageWeight[MAX_AIRLINES];		/* Weight of baggage on a single airline */

int boardingLounges[MAX_AIRLINES];		/* Array of count of people waiting in airport lounge for airline to leave */
int totalPassengersOfAirline[MAX_AIRLINES];		/* Total passengers that should be on an airline */
int totalBaggage[MAX_AIRLINES];
int totalWeight[MAX_AIRLINES];
int ticketsIssued[MAX_AIRLINES];
int liaisonBaggageCount[MAX_AIRLINES];			/* baggage count from liaison's perspective, per each airline */
int alreadyBoarded[MAX_AIRLINES];
int execLineNeedsHelp[MAX_AIRLINES];

int simNumOfPassengers;
int simNumOfAirlines ;
int simNumOfLiaisons;
int simNumOfCIOs;
int simNumOfCargoHandlers;
int simNumOfScreeningOfficers;
int simSeatsPerPlane = 4;
int planeCount = 0;
Passenger_t simPassengers[PASSENGER_COUNT];
LiaisonOfficer_t simLiaisons[MAX_LIAISONS];
CheckInOfficer_t simCIOs[MAX_CIOS];
CargoHandler_t simCargoHandlers[MAX_CARGOHANDLERS];
ScreeningOfficer_t simScreeningOfficers[MAX_SCREEN];
SecurityOfficer_t simSecurityOfficers[MAX_SCREEN];
AirportManager_t simAirportManager;

/*----------------------------------------------------------------------
// Passenger
//----------------------------------------------------------------------*/
void CheckIn_DoWork(int number);
void Manager_DoWork();
void EndOfDay();
void Screening_DoWork(int n);
void Screening_setBusy(int n);
void Security_DoWork(int number);

void Passenger(){		/* Picks a Liaison line, talks to the Officer, gets airline */
	int i, test, r, oldLine, name;
	Acquire(Passenger_ID_Lock);	
	name = Passenger_ID;
		printf((int)"STARTED PASS %d\n", sizeof("STARTED PASS %d\n"), name, 0);

	simPassengers[name].name = name;
	Passenger_ID++;
	Release(Passenger_ID_Lock);
	Acquire(liaisonLineLock);		/* Acquire lock to find shortest line */
	simPassengers[name].myLine = 0;	

printf((int)"Passenger choosing liai line\n", sizeof("Passenger choosing liai line\n"), 0, 0);		/* OFFICIAL OUTPUT STATEMENT */

	for(i = 1; i < simNumOfLiaisons; i++){		/* Find shortest line */
		if(liaisonLine[i] < liaisonLine[simPassengers[name].myLine]){
			simPassengers[name].myLine = i;
		}
	}

	printf((int)"Passenger %d chose Liaison %d ", sizeof("Passenger %d chose Liaison %d "), name, simPassengers[name].myLine);		/* OFFICIAL OUTPUT STATEMENT */
	printf((int)"with a line of length %d\n", sizeof("with a line of length %d\n"), liaisonLine[simPassengers[name].myLine], 0);
	
	liaisonLine[simPassengers[name].myLine] += 1;		/* Increment size of line you join*/
	Wait(liaisonLineCV[simPassengers[name].myLine], liaisonLineLock);
	Acquire(liaisonLineLocks[simPassengers[name].myLine]); /* New lock needed for liaison interaction */
	Release(liaisonLineLock);		/* Release the lock you acquired from waking up */
	printf((int)"PASS ACQUIRED %d\n", sizeof("PASS ACQUIRED %d\n"), liaisonLineLocks[simPassengers[name].myLine], 0);
	LPInfo[simPassengers[name].myLine].passengerName = simPassengers[name].name;
	LPInfo[simPassengers[name].myLine].baggageCount = simPassengers[name].baggageCount; /* Adds baggage Count to shared struct array */
	Signal(liaisonOfficerCV[simPassengers[name].myLine], liaisonLineLocks[simPassengers[name].myLine]); /* Wakes up Liaison Officer */
	printf((int)"Signaled Liai, PASS %d\n", sizeof("Signaled Liai, PASS %d\n"), name, 0);
	Wait(liaisonOfficerCV[simPassengers[name].myLine], liaisonLineLocks[simPassengers[name].myLine]); /* Goes to sleep until Liaison finishes assigning airline */
	simPassengers[name].airline = LPInfo[simPassengers[name].myLine].airline;		/* Gets airline info from Liaison Officer shared struct */
	totalBaggage[simPassengers[name].airline] += simPassengers[name].baggageCount;
	
	for(i = 0; i < simPassengers[name].baggageCount; i++){			/* Add baggage and their weight to Passenger Baggage Vector */
		totalWeight[simPassengers[name].airline] += simPassengers[name].bags[i].weight;
	}
	
	if (liaisonLine[simPassengers[name].myLine] > 0) liaisonLine[simPassengers[name].myLine]--; /*Passenger left the line */
	Signal(liaisonOfficerCV[simPassengers[name].myLine], liaisonLineLocks[simPassengers[name].myLine]); /* Wakes up Liaison Officer to say I'm leaving */
	Wait(liaisonOfficerCV[simPassengers[name].myLine], liaisonLineLocks[simPassengers[name].myLine]);	/* wait for Liaison to direct me */
	Release(liaisonLineLocks[simPassengers[name].myLine]); /* Passenger is now leaving to go to airline checking */	

	printf((int)"Passenger %d of Airline %d is directed to the check-in counter\n", sizeof("Passenger %d of Airline %d is directed to the check-in counter\n"), name, simPassengers[name].airline);		/* OFFICIAL OUTPUT STATEMENT */
	
	Yield();
/* ----------------------------------------------------[ Going to CheckIn ]---------------------------------------- */

	Acquire(CheckInLock);		/* Acquire lock to find shortest line */
	if(simPassengers[name].economy == 0){		/* If executive class */
		simPassengers[name].myLine = simNumOfCIOs * simNumOfAirlines + simPassengers[name].airline; /* There are always airline count more executive lines than economy lines */
		printf((int)"Passenger %d of Airline %d is waiting in the executive class line\n", sizeof("Passenger %d of Airline %d is waiting in the executive class line\n"), name, simPassengers[name].airline);		/* OFFICIAL OUTPUT STATEMENT */
	} else { 
		simPassengers[name].myLine = (simPassengers[name].airline)*simNumOfCIOs;		/* Economy lines are in intervals of Check In Count */
		for (i = (simPassengers[name].airline)*simNumOfCIOs; i < simPassengers[name].airline * simNumOfCIOs + simNumOfCIOs; i++){ /* Find shortest line for my airline */
			if (CheckInLine[i] < CheckInLine[simPassengers[name].myLine]) simPassengers[name].myLine = i;
		}
		printf((int)"Passenger %d of Airline %d chose Airline Check-In staff ", sizeof("Passenger %d of Airline %d chose Airline Check-In staff "), name, simPassengers[name].airline/*, CheckInLine[simPassengers[name].myLine]*/);		/* OFFICIAL OUTPUT STATEMENT */
		printf((int)"%d with a line length %d\n", sizeof("%d with a line length %d\n"), simPassengers[name].myLine, CheckInLine[simPassengers[name].myLine]);
	}

	CheckInLine[simPassengers[name].myLine] += 1;		/* Increment line when I enter line */
	if (simPassengers[name].economy && CheckInLine[simPassengers[name].myLine] == 1){		/* If first person in line for economy passages, message the officer (they may be on break) */
		Signal(CheckInBreakCV[simPassengers[name].myLine], CheckInLock);
	} else if (simPassengers[name].economy == 0 && CheckInLine[simPassengers[name].myLine] == 1){		/* If first person in line for executive passengers */
		for (i = simPassengers[name].airline * simNumOfCIOs; i < simPassengers[name].airline * simNumOfCIOs + simNumOfCIOs; i++){
			if (CheckInLine[i] == 0){
				Signal(CheckInBreakCV[i], CheckInLock);	/* Message all Officers with no line (they may be on break) */
			}
		}
	}
	CPInfo[simPassengers[name].myLine].passenger = simPassengers[name].name;		/* Tell Officer passenger name */
	Wait(CheckInCV[simPassengers[name].myLine], CheckInLock);		/* Wait for CheckIn Officer to signal */
	/* int oldLine = myLine;		// Save old line to decrement it later when you leave */
	if(simPassengers[name].economy == 0){
		/* execLineLocks[airline]->Acquire(); */
		simPassengers[name].myLine = CPInfo[simPassengers[name].myLine].line;		/* Sets its line to that of the Officer if it was in the executive line
		// execLineCV[airline]->Signal(execLineLocks[airline]);
		// execLineLocks[airline]->Release(); */
	}
	Acquire(CheckInLocks[simPassengers[name].myLine]);
	Release(CheckInLock);
	
	CPInfo[simPassengers[name].myLine].baggageCount = simPassengers[name].baggageCount;		/* Place baggage onto counter (into shared struct) */
	for (i = 0; i < simPassengers[name].baggageCount; i++){		/* Add baggage info to shared struct for CheckIn Officer */
		CPInfo[simPassengers[name].myLine].bag[i] = simPassengers[name].bags[i];
	}
	
	CPInfo[simPassengers[name].myLine].passenger = simPassengers[name].name;		/* Tell Officer passenger name */
	CPInfo[simPassengers[name].myLine].IsEconomy = simPassengers[name].economy;		/* Tell Officer passenger class */
	Signal(CheckInOfficerCV[simPassengers[name].myLine], CheckInLocks[simPassengers[name].myLine]);		/* Wake up CheckIn Officer */
	Wait(CheckInOfficerCV[simPassengers[name].myLine], CheckInLocks[simPassengers[name].myLine]);
	simPassengers[name].seat = CPInfo[simPassengers[name].myLine].seat;		/* Get seat number from shared struct */
	simPassengers[name].gate = simPassengers[name].airline;
	
	if (CheckInLine[simPassengers[name].myLine] > 0) CheckInLine[simPassengers[name].myLine]--; /* Passenger left the line */
	Signal(CheckInOfficerCV[simPassengers[name].myLine], CheckInLocks[simPassengers[name].myLine]); /* Wakes up CheckIn Officer to say I'm leaving */
	printf((int)"Passenger %d of Airline %d ", sizeof("Passenger %d of Airline %d "), name, simPassengers[name].airline);		/*OFFICIAL OUTPUT STATEMENT */
	printf((int)"was informed to board at gate %d\n", sizeof("was informed to board at gate %d\n"), simPassengers[name].gate, 0);
	Release(CheckInLocks[simPassengers[name].myLine]); /* Passenger is now leaving to go to screening */
	
	Yield();	
	
/* ----------------------------------------------------[ Going to Screening ]---------------------------------------- */

	Acquire(ScreenLines);
	
	test = true;
	if (ScreenLine[0] == 0){		/* If first person in line for Screening Officer... */
		ScreenLine[0] += 1;		/* Increment line length.... */
		Release(ScreenLines);
		while (test){		/* Look for a Screening Officer that isn't busy... (and keep going till you find one) */
			Acquire(ScreenLines);
			for (i = 0; i < simNumOfScreeningOfficers; i++){
				if(Screen[i].IsBusy == 0){		/* This checks if that officer is busy */
					test = Screen[i].IsBusy;		
					simPassengers[name].myLine = i;
				}
			}
			Release(ScreenLines);
			Yield();		/* Wait a bit before re looping to give officer time to change his busy state */
		}
		Acquire(ScreenLines);
	} else {		/* If not first person in line, just add yourself to the line */
		ScreenLine[0] += 1;
		Wait(ScreenLineCV[0], ScreenLines);
	}
	if (test){
		for (i = 0; i < simNumOfScreeningOfficers; i++){
			if(!(Screen[i].IsBusy)){
				test = Screen[i].IsBusy;
				simPassengers[name].myLine = i;
			}
		}
	}
	
	Screening_setBusy(simPassengers[name].myLine);		/* Set the officer that you picked to busy */
	Acquire(ScreenLocks[simPassengers[name].myLine]);
	Release(ScreenLines);		/* Release the lock so others can access officer busy states */
	printf((int)"Passenger %d gives the hand-luggage to screening officer %d\n", sizeof("Passenger %d gives the hand-luggage to screening officer %d\n"), name, simPassengers[name].myLine);		/* OFFICIAL OUTPUT STATEMENT */
	SPInfo[simPassengers[name].myLine].passenger = simPassengers[name].name;		/* Tell Screening Officer your name */
	Signal(ScreenOfficerCV[simPassengers[name].myLine], ScreenLocks[simPassengers[name].myLine]);		/* Wake them up */
	Wait(ScreenOfficerCV[simPassengers[name].myLine], ScreenLocks[simPassengers[name].myLine]);		/* Go to sleep */
	
	oldLine = simPassengers[name].myLine;		/* Save old line to signal to Screening you are leaving */
	simPassengers[name].myLine = SPInfo[oldLine].SecurityOfficer;		/* Receive line of which Security Officer to wait for */
	Signal(ScreenOfficerCV[oldLine], ScreenLocks[oldLine]);		/* Tells Screening Officer Passenger is leaving */
	if(ScreenLine[0] > 0) ScreenLine[0] -= 1;		/* Decrement line size */
	Release(ScreenLocks[oldLine]);
	
	/* currentThread->Yield(); */
	
/* ----------------------------------------------------[ Going to Security ]---------------------------------------- */

	Acquire(SecurityLines);
	SecurityLine[simPassengers[name].myLine]++;		/* Increase line length... */
	Acquire(SecurityLocks[simPassengers[name].myLine]);
	printf((int)"Passenger %d moves to security inspector %d\n", sizeof("Passenger %d moves to security inspector %d\n"), name, simPassengers[name].myLine);		/* OFFICIAL OUTPUT STATEMENT */

	Release(SecurityLines);
	Signal(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* Tell Security that you have arrived */
	Wait(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);
	SecPInfo[simPassengers[name].myLine].passenger = simPassengers[name].name;		/* Tell officer passenger name */
	SecPInfo[simPassengers[name].myLine].questioning = false;		/* Tell officer that passenger hasn't been told to do extra questioning */
	Signal(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* Tell Security that you have arrived */
	Wait(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);
	simPassengers[name].NotTerrorist = SecPInfo[simPassengers[name].myLine].PassedSecurity;		/* Boolean of whether the passenger has passed all security */
	
	Acquire(SecurityLines);
	SecurityLine[simPassengers[name].myLine] -= 1;		/* Decrement Line Size */
	Release(SecurityLines);
	Signal(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* Tell Security officer that passenger is going to boarding area or questioning */
	Release(SecurityLocks[simPassengers[name].myLine]);	/* Go to boarding area or questioning */

	if (simPassengers[name].NotTerrorist == 0){		/* If they failed, then go to questioning */	
		printf((int)"Passenger %d goes for further questioning\n", sizeof("Passenger %d goes for further questioning\n"), name, -1);		/* OFFICIAL OUTPUT STATEMENT */
		r = rand() % 2 + 1;		/* Random length of questioning */
		for (i = 0; i < r; i++){		/* Stay for questioning for a random length of time */
			Yield();
		}
		
		Acquire(SecurityLines);		/* Lock for waiting in Line*/
		Acquire(SecurityAvail);
		SecurityLine[simPassengers[name].myLine] += 1;		/* Add yourself to line length... */
		SecurityAvailability[simPassengers[name].myLine] = false;
		Release(SecurityAvail);
		Acquire(SecurityLocks[simPassengers[name].myLine]);
		if (SecurityLine[simPassengers[name].myLine] == 1){		/* If there is no line when you return... */
			Signal(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* And signal the officer (they are probably sleeping)*/
			printf((int)"Signaled officer\n", sizeof("Signaled officer\n"), 0, 0);
		}else {		/* Otherwise just add yourself to the line and wait (Security Officer will not take anymore passengers as you are in queue) */
			Wait(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* Wait on security officer to wake you up */
			Signal(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* Tell Security that you have returned from questioning */
		}
		Release(SecurityLines);
		Wait(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* Wait on security officer to wake you up */
		SecPInfo[simPassengers[name].myLine].passenger = simPassengers[name].name;		/* Tell Security Officer passenger name again */
		SecPInfo[simPassengers[name].myLine].questioning = true;		/* Tell officer passenger already underwent questioning and should auto pass */
		printf((int)"Passenger %d comes back to security inspector %d after further examination\n", sizeof("Passenger %d comes back to security inspector %d after further examination\n"), name, simPassengers[name].myLine);		/* OFFICIAL OUTPUT STATEMENT */
		Signal(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		/* Tell Security that you have returned from questioning */
		Wait(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);
		Acquire(SecurityLines);
		SecurityLine[simPassengers[name].myLine] -= 1;		/* Leave the line */
		Signal(SecurityOfficerCV[simPassengers[name].myLine], SecurityLocks[simPassengers[name].myLine]);		
		Release(SecurityLocks[simPassengers[name].myLine]);		/* Go To Boarding Area */
		Release(SecurityLines);
	}
	
/* ----------------------------------------------------[ Going to Gate ]---------------------------------------- */
	
	Acquire(gateLocks[simPassengers[name].airline]);		/* Lock to control passenger flow to boarding lounge */
	printf((int)"Passenger %d of Airline %d ", sizeof("Passenger %d of Airline %d "), name, simPassengers[name].airline);		/* OFFICIAL OUTPUT STATEMENT */
	printf((int)"reached the gate %d\n", sizeof("reached the gate %d\n"), simPassengers[name].gate, 0);
	boardingLounges[simPassengers[name].airline]++;

	Wait(gateLocksCV[simPassengers[name].airline], gateLocks[simPassengers[name].airline]);		/* Airline can only leave when all passengers have arrived */
	printf((int)"Passenger %d of Airline %d ", sizeof("Passenger %d of Airline %d "), name, simPassengers[name].airline);		/* OFFICIAL OUTPUT STATEMENT */
	printf((int)"boarded airline %d\n", sizeof("boarded airline %d\n"), simPassengers[name].airline,0);
	Release(gateLocks[simPassengers[name].airline]);
	Exit(0);
}

/*----------------------------------------------------------------------
// Liaison Officer
//---------------------------------------------------------------------- */
int Liaison_getPassengerCount(int n) {return liaisonOfficers[n].passengerCount;} /* For manager to get passenger headcount */

int Liaison_getAirlineBaggageCount(int n){ /* For manager to get passenger bag count */
	return liaisonOfficers[n].airlineBaggageCount[n];
}

void Liaison(){
	int name;
	printf((int)"Started Liaison %d\n", sizeof("Started Liaison %d\n"), name, 0);		/* OFFICIAL OUTPUT STATEMENT */

	Acquire(Liaison_ID_Lock);
	name = Liaison_ID;
	liaisonOfficers[name].number = name;
	Liaison_ID++;
	Release(Liaison_ID_Lock);
					printf((int)"LIAI %d ABOUT TO WHILE\n", sizeof("LIAI %d ABOUT TO WHILE\n"), name, 0);		/* OFFICIAL OUTPUT STATEMENT */

	while(true){		/* Always be running, never go on break */
			/* printf((int)"LIAI %d\n", sizeof("LIAI %d\n"), name, 0); */

		Acquire(liaisonLineLock);		/* Acquire lock for lining up in order to see if there is someone waiting in your line */
		if (liaisonLine[name] > 0){		/* Check if passengers are in your line */
		printf((int)"LiaiLine %d = %d\n", sizeof("LiaiLine %d = %d\n"), name, liaisonLine[name]);		/* OFFICIAL OUTPUT STATEMENT */
			Signal(liaisonLineCV[name], liaisonLineLock);		/* Signal them if there are */
			Acquire(liaisonLineLocks[name]);		
			Release(liaisonLineLock);
			printf((int)"LIAI ACQUIRED %d\n", sizeof("LIAI ACQUIRED %d\n"), liaisonLineLocks[name], 0);
			
			Wait(liaisonOfficerCV[name], liaisonLineLocks[name]);		/* Wait for passenger to give you baggage info */
			
			printf((int)"GOT BAGGAGE INFO LIAI %d\n", sizeof("GOT BAGGAGE INFO LIAI %d\n"), name, 0);
			
			/* Passenger has given bag Count info and woken up the Liaison Officer */
			liaisonOfficers[name].passengerCount += 1;		/*Increment internal passenger counter */
			Acquire(seatLock);
			while(true){
				liaisonOfficers[name].airline = rand() % simNumOfAirlines;
				if (ticketsIssued[liaisonOfficers[name].airline] < totalPassengersOfAirline[liaisonOfficers[name].airline]){
					ticketsIssued[liaisonOfficers[name].airline] += 1;
					break;
				}
			}
			liaisonOfficers[name].airlineBaggageCount[liaisonOfficers[name].airline] += LPInfo[liaisonOfficers[name].number].baggageCount;
			Release(seatLock);
			LPInfo[liaisonOfficers[name].number].airline = liaisonOfficers[name].airline;		/* Put airline number in shared struct for passenger */
			liaisonBaggageCount[liaisonOfficers[name].airline] += LPInfo[liaisonOfficers[name].number].baggageCount;
			Signal(liaisonOfficerCV[name], liaisonLineLocks[name]); /* Wakes up passenger */
			Wait(liaisonOfficerCV[name], liaisonLineLocks[name]); /* Waits for Passenger to say they are leaving */
			printf((int)"Airport Liaison %d directed passenger %d ", sizeof("Airport Liaison %d directed passenger %d "), name, LPInfo[name].passengerName);		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"of airline %d\n", sizeof("of airline %d\n"), liaisonOfficers[name].airline,0);
			Signal(liaisonOfficerCV[name], liaisonLineLocks[name]);	/*Let the passenger leave */
			Release(liaisonLineLocks[name]);
		}
		else {
			Release(liaisonLineLock); /*if there are no passengers in line, release */
			Yield();
			if(planeCount == simNumOfAirlines){
				break;
			}
		}
		Yield();
	}
}

/*----------------------------------------------------------------------
// Check In Staff
//---------------------------------------------------------------------- */

void CheckInOfficer(){
	int i;
	Acquire(CheckIn_ID_Lock);	
	i = CheckIn_ID;
	CheckIn[i].number = i;
	CheckIn_ID++;
	Release(CheckIn_ID_Lock);
	CheckIn_DoWork(i);
}

int CheckIn_getAirline(int n){return CheckIn[n].airline;}
int CheckIn_getNumber(int n) {return CheckIn[n].number;}
int CheckIn_getOnBreak(int n) {return CheckIn[n].OnBreak;}
void CheckIn_setOffBreak(int n) {CheckIn[n].OnBreak = false;}

void CheckIn_DoWork(int number){
printf((int)"Started CIO %d\n", sizeof("Started CIO %d\n"), number, 0);
	while(CheckIn[number].work){		/* While there are still passengers who haven't checked in */
		int i, x, y, helpedExecLine;
		helpedExecLine = false;
		Acquire(CheckInLock);		/* Acquire line lock to see if there are passengers in line */
		x = simNumOfAirlines*simNumOfCIOs + CheckIn[number].airline;		/* Check Executive Line for your airline first */
		if (CheckIn[number].OnBreak == 1) CheckIn[number].OnBreak = false;
		/* execLineLocks[info.airline]->Acquire(); */
		if(CheckInLine[x] > 0 && execLineNeedsHelp[CheckIn[number].airline]){
			execLineNeedsHelp[CheckIn[number].airline] = false;
			helpedExecLine  = true;
			CPInfo[x].line = CheckIn[number].number;
			CheckInLine[x]--;
			Signal(CheckInCV[x], CheckInLock);
			printf((int)"Airline check-in staff %d of airline %d serves an executive class passenger and economy class line length = ", sizeof("Airline check-in staff %d of airline %d serves an executive class passenger and economy class line length = "), number, CheckIn[number].airline);		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"%d\n", sizeof("%d\n"), CheckInLine[CheckIn[number].number], 0);
			CheckInLine[CheckIn[number].number]++;
			/* execLineCV[info.airline]->Wait(execLineLocks[info.airline]); */
		} else if (CheckInLine[CheckIn[number].number] > 0){		/* If no executive, then check your normal line */
			Signal(CheckInCV[CheckIn[number].number], CheckInLock);
			printf((int)"Airline check-in staff %d of airline %d serves an economy class passenger and executive class line length = ", sizeof("Airline check-in staff %d of airline %d serves an economy class passenger and executive class line length = "), number, CheckIn[number].airline);		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"%d\n", sizeof("%d\n"), CheckInLine[x], 0);
		}else {		/* Else, there are no passengers waiting and you can go on break */
			/* if(execLineLocks[info.airline]->isHeldByCurrentThread()){
				// execLineLocks[info.airline]->Release();
			// } */
			CheckIn[number].OnBreak = true;
			Wait(CheckInBreakCV[CheckIn[number].number], CheckInLock);
			Release(CheckInLock);
			continue;
		}
		/* if(execLineLocks[info.airline]->isHeldByCurrentThread()){
			// execLineLocks[info.airline]->Release();
		// } */
		CheckIn[number].passengerCount++;
		Acquire(CheckInLocks[CheckIn[number].number]);
		Release(CheckInLock);
		Wait(CheckInOfficerCV[CheckIn[number].number], CheckInLocks[CheckIn[number].number]);		/* Wait for passenger to give you baggage and airline */
		if(helpedExecLine){
			execLineNeedsHelp[CheckIn[number].airline] = true;
		}
		for(i = 0; i < CPInfo[CheckIn[number].number].baggageCount; i++){
			if(CPInfo[CheckIn[number].number].bag[i].weight != 0){
				CheckIn[number].bags[i] = CPInfo[CheckIn[number].number].bag[i];		/* Place baggage info in shared struct into internal struct */
			} else {
				CheckIn[number].bags[i].weight = 0;
			}
		}
		
		for (i = 0; i < CPInfo[CheckIn[number].number].baggageCount; i++){	
			CheckIn[number].bags[i].airline = CheckIn[number].airline;		/* Add airline code to baggage */
			Acquire(BaggageLock);			/* Acquire lock for putting baggage on conveyor */
			
			for(y = 0; y < PASSENGER_COUNT*2; y++){
				if(conveyor[y].weight == 0){
					conveyor[y] = CheckIn[number].bags[i];
					printf((int)"Airline check-in staff %d of airline %d dropped bags to the conveyor system\n", sizeof("Airline check-in staff %d of airline %d dropped bags to the conveyor system\n"), number, CheckIn[number].airline);		/* OFFICIAL OUTPUT STATEMENT */
					printf((int)"CCCCOOOOOOONNNNNNNNNNNNNVVVVVVEEEEEEEEEEEYYYYYYYOOOOOOOOORRRRRRRRRRRR[%d] has weight %d\n", sizeof("CCCCOOOOOOONNNNNNNNNNNNNVVVVVVEEEEEEEEEEEYYYYYYYOOOOOOOOORRRRRRRRRRRR[%d] has weight %d\n"), y, conveyor[y].weight);
					printf((int)"%d is supposed weight\n", sizeof("%d is supposed weight\n"), CheckIn[number].bags[i].weight, 0);
					break;
				}
			}
			
			/*conveyor.push_back(CheckIn[number].bags[i]);		/* Place baggage onto conveyor belt for Cargo Handlers */
			Release(BaggageLock);		/* Release baggage lock */
			CheckIn[number].totalBags[(CheckIn[number].passengerCount - 2) + i] = (CheckIn[number].bags[i]);
		}
		
		CPInfo[CheckIn[number].number].gate = gates[CheckIn[number].airline];		/* Tell Passenger Gate Number*/
		if (!CPInfo[CheckIn[number].number].IsEconomy){
			printf((int)"Airline check-in staff %d of airline %d ", sizeof("Airline check-in staff %d of airline %d "), number, CheckIn[number].airline);		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"informs executive class passenger %d to board at gate %d\n", sizeof("informs executive class passenger %d to board at gate %d\n"), CPInfo[number].passenger, gates[CheckIn[number].airline]);
		} else {
			printf((int)"Airline check-in staff %d of airline %d ", sizeof("Airline check-in staff %d of airline %d "), number, CheckIn[number].airline);		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"informs economy class passenger %d to board at gate %d\n", sizeof("informs economy class passenger %d to board at gate %d\n"), CPInfo[number].passenger, gates[CheckIn[number].airline]);
		}
		Signal(CheckInOfficerCV[CheckIn[number].number], CheckInLocks[CheckIn[number].number]);
		Wait(CheckInOfficerCV[CheckIn[number].number], CheckInLocks[CheckIn[number].number]);		/* Passenger will wake up you when they leave, starting the loop over again */
		Release(CheckInLocks[CheckIn[number].number]);
		Yield();
	}
	printf((int)"Airline check-in staff %d is closing the counter\n", sizeof("Airline check-in staff %d is closing the counter\n"), CheckIn[number].number, -1);		/* OFFICIAL OUTPUT STATEMENT */
}

/*----------------------------------------------------------------------
// Cargo Handler
//---------------------------------------------------------------------- */
void Cargo(){
	int i, n;
	Baggage_t temp;		/* Baggage that handler will move off conveyor */
	Acquire(Cargo_ID_Lock);	
	n = Cargo_ID;
	cargoHandlers[n].name = n;
	Cargo_ID++;
	Release(Cargo_ID_Lock);
	printf((int)"Started CARGO %d\n", sizeof("Started CARGO %d\n"), n, 0);
	while (true){
		temp.weight = 0;
		cargoHandlers[n].onBreak = false;
		Acquire(CargoHandlerLock);
		for (i = 0; i < PASSENGER_COUNT*2; i++){
			if(conveyor[i].weight != 0){
				temp = conveyor[i];
				conveyor[i].weight = 0;
				break;
			}
		}
		if (temp.weight == 0){
			cargoHandlers[n].onBreak = true;
			printf((int)"Cargo Handler %d is going for a break\n", sizeof("Cargo Handler %d is going for a break\n"), n, -1);		/* OFFICIAL OUTPUT STATEMENT */
			Wait(CargoHandlerCV, CargoHandlerLock);		/* Sleep until woken up by manager */
			Release(CargoHandlerLock);
			printf((int)"Cargo Handler %d returned from break\n", sizeof("Cargo Handler %d returned from break\n"), n, -1);		/* OFFICIAL OUTPUT STATEMENT */
			continue;
		}
	
		Acquire(AirlineBaggage[temp.airline]);		/* Acquire lock to put baggage on an airline */
		Release(CargoHandlerLock);
		aircraftBaggageCount[temp.airline]++;		/* Increase baggage count of airline */
		aircraftBaggageWeight[temp.airline] += temp.weight;		/* Increase baggage weight of airline */
		printf((int)"Cargo Handler %d picked bag of airline %d ", sizeof("Cargo Handler %d picked bag of airline %d "), n, temp.airline);		/* OFFICIAL OUTPUT STATEMENT */
		printf((int)"with weighing %d lbs\n", sizeof("with weighing %d lbs\n"), temp.weight, 0);
		printf((int)"ID is: %d\n", sizeof("ID is: %d\n"), Cargo_ID, -1);
		printf((int)"number is: %d\n", sizeof("number is: %d\n"), n, -1);
		cargoHandlers[n].weight[temp.airline] += temp.weight;		/* Increment total weight of baggage this handler has dealt with */
		cargoHandlers[n].count[temp.airline] ++;		/* Increment total count of baggage this handler has dealt with */
		Release(AirlineBaggage[temp.airline]);
		temp.weight = 0;
		Yield();
	}
}

/*----------------------------------------------------------------------
// Airport Manager
//---------------------------------------------------------------------- */

void AirportManager(){
	int i;
	for(i =0; i < simNumOfAirlines; i++){
		simAirportManager.CIOTotalCount[i] = 0;
		simAirportManager.CIOTotalWeight[i] = 0;
		simAirportManager.LiaisonTotalCount[i] = 0;
		simAirportManager.CargoHandlerTotalWeight[i] = 0;
		simAirportManager.CargoHandlerTotalCount[i] = 0;
	}
	simAirportManager.liaisonPassengerCount = 0;
	simAirportManager.checkInPassengerCount = 0;
	simAirportManager.securityPassengerCount = 0;
	Manager_DoWork();
}

void Manager_DoWork(){
	int breakCount, y;
	while(true){
		int i;
		for (i = 0; i < PASSENGER_COUNT*2; i++){
			if(conveyor[i].weight != 0){
				Acquire(CargoHandlerLock);
				breakCount = 0;
				for(y = 0; y < simNumOfCargoHandlers; y++){
				printf((int)"CH %d on break? %d\n", sizeof("CH %d on break? %d\n"), y, cargoHandlers[y].onBreak);
					if(cargoHandlers[y].onBreak == 1){
						breakCount++;
					}	
				}
				if(breakCount == simNumOfCargoHandlers){
					printf((int)"Airport manager calls back all the cargo handlers from break\n", sizeof("Airport manager calls back all the cargo handlers from break\n"), -1, -1);
					Broadcast(CargoHandlerCV, CargoHandlerLock);
					Release(CargoHandlerLock);
					break;
				}
				Release(CargoHandlerLock);
			}
		}
		
		for(i = 0; i < simNumOfAirlines; i++){
			printf("Boarding Lounge %d has passengers: %d\n", sizeof("Boarding Lounge %d has passengers: %d\n"), i, boardingLounges[i]);
			printf("Total Baggage for %d has: %d\n", sizeof("Total Baggage for %d has: %d\n"), i, totalBaggage[i]);
			printf("Aircraft Baggage for %d has: %d\n", sizeof("Aircraft Baggage for %d has: %d\n"), i, aircraftBaggageCount[i]);
			if(boardingLounges[i] == totalPassengersOfAirline[i] && totalBaggage[i] == aircraftBaggageCount[i] && !alreadyBoarded[i]){
				Acquire(gateLocks[i]);
				Broadcast(gateLocksCV[i], gateLocks[i]);
				alreadyBoarded[i] = true;
				Release(gateLocks[i]);
				planeCount++;
			}
		}
		if(planeCount == simNumOfAirlines){
			Yield();
			EndOfDay();
			break;
		}
		for (i = 0; i < 2; i++){
			Yield();
		}
	}
}

void EndOfDay(){
	int i, j;
	for(i = 0; i < simNumOfCargoHandlers; i++){
		for(j = 0; j < simNumOfAirlines; j++){
			simAirportManager.CargoHandlerTotalWeight[j] += cargoHandlers[i].weight[j];
			simAirportManager.CargoHandlerTotalCount[j] += cargoHandlers[i].count[j];
		}
	}
	for(i = 0; i < simNumOfCIOs * simNumOfAirlines; i++){
		for(j = 0; j < PASSENGER_COUNT*2; j++){
			simAirportManager.CIOTotalWeight[CheckIn[i].totalBags[j].airline] += CheckIn[i].totalBags[j].weight;
		}
		simAirportManager.checkInPassengerCount += CheckIn[i].passengerCount;
	}

	for(i = 0; i < simNumOfLiaisons; i++){
		for(j = 0; j < simNumOfAirlines; j++){
			simAirportManager.LiaisonTotalCount[j] += liaisonOfficers[i].airlineBaggageCount[j];
		}
		simAirportManager.liaisonPassengerCount += liaisonOfficers[i].passengerCount;
	}
	
	for(i = 0; i < simNumOfScreeningOfficers; i++){
		simAirportManager.securityPassengerCount += Security[i].PassedPassengers;
	}
	printf((int)"Passenger count reported by airport liaison = %d\n", sizeof("Passenger count reported by airport liaison = %d\n"), simAirportManager.liaisonPassengerCount, -1);
	printf((int)"Passenger count reported by airport liaison = %d\n", sizeof("Passenger count reported by airport liaison = %d\n"), simAirportManager.checkInPassengerCount, -1);
	printf((int)"Passenger count reported by security inspector = %d\n", sizeof("Passenger count reported by security inspector = %d\n"), simAirportManager.securityPassengerCount, -1);
	
	for(i = 0; i < simNumOfAirlines; i++){
		printf((int)"From setup: Baggage count of airline %d = %d\n", sizeof("From setup: Baggage count of airline %s = %d\n"), i, totalBaggage[i]);
		printf((int)"From airport liaison: Baggage count of airline %d = %d\n", sizeof("From airport liaison: Baggage count of airline %d = %d\n"), i, simAirportManager.LiaisonTotalCount[i]);
		printf((int)"From cargo handlers: Baggage count of airline %d = %d\n", sizeof("From cargo handlers: Baggage count of airline %d = %d\n"), i, simAirportManager.CargoHandlerTotalCount[i]);
		printf((int)"From setup: Baggage weight of airline %d = %d\n", sizeof("From setup: Baggage weight of airline %d = %d\n"), i, totalWeight[i]);
		printf((int)"From airline check-in staff: Baggage weight of airline %d = %d\n", sizeof("From airline check-in staff: Baggage weight of airline %d = %d\n"), i, simAirportManager.CIOTotalWeight[i]);
		printf((int)"From cargo handlers: Baggage weight of airline %d = %d\n", sizeof("From cargo handlers: Baggage weight of airline %d = %d\n"), i, simAirportManager.CargoHandlerTotalWeight[i]);
	}
}

/*----------------------------------------------------------------------
// Screening Officer
//---------------------------------------------------------------------- */
void ScreeningOfficer(){
	int i;
	Acquire(Screening_ID_Lock);	
	i = Screening_ID;
	Screen[i].number = i;
	Screening_ID++;
	Release(Screening_ID_Lock);
	
	Acquire(ScreenLines);
	Screen[i].IsBusy = false;		/* Set Officer to available */
	Release(ScreenLines);
	Screening_DoWork(i);
}

void Screening_setBusy(int n){
	Acquire(ScreenLines);
	Screen[n].IsBusy = true;
	Release(ScreenLines);
}

void Screening_DoWork(int n){
printf((int)"Started Screening %d\n", sizeof("Started Screening %d\n"), n, 0);
	while(true){
		int i, y, z, x, alreadyPrinted;
		Acquire(ScreenLines);
		if (Screen[n].IsBusy) Screen[n].IsBusy = false;		/* If busy, should no longer be busy */
		if (ScreenLine[0] > 0){		/* Checks if the screening line has passengers */
			Signal(ScreenLineCV[0], ScreenLines);		/* Wake them if there are */
		}
		Acquire(ScreenLocks[Screen[n].number]);
		Release(ScreenLines);
		Wait(ScreenOfficerCV[Screen[n].number], ScreenLocks[Screen[n].number]);		/* Wait for Passenger to start conversation */

		z = SPInfo[Screen[n].number].passenger;		/* Find passenger name */
		x = rand() % 5;		/* Generate random value for pass/fail */
		Screen[n].ScreenPass = true;		/* Default is pass */
		if (x == 0) Screen[n].ScreenPass = false;		/* 20% of failure */
		ScreeningResult[z] = Screen[n].ScreenPass;
		if (Screen[n].ScreenPass){		/* If passenger passed test */
			printf((int)"Screening officer %d is not suspicious of the hand luggage of passenger %d\n", sizeof("Screening officer %d is not suspicious of the hand luggage of passenger %d\n"), n, z);		/* OFFICIAL OUTPUT STATEMENT */
		}else {
			printf((int)"Screening officer %d is suspicious of the hand luggage of passenger %d\n", sizeof("Screening officer %d is suspicious of the hand luggage of passenger %d\n"), n, z);		/* OFFICIAL OUTPUT STATEMENT */
		}
		alreadyPrinted = false;
		while(true){		/* Wait for Security Officer to become available */
			y = false;
			for (i = 0; i < simNumOfScreeningOfficers; i++){		/* Iterate through all security officers */
				Acquire(SecurityAvail);
				y = SecurityAvailability[i];		/* See if they are busy */
				if (y){			/* If a security officer is not busy, obtain his number and inform passenger */
					SPInfo[Screen[n].number].SecurityOfficer = i;
					SecurityAvailability[i] = false;
				}
				Release(SecurityAvail);
			}
			if(y){
				break;
			}
			alreadyPrinted = true;
			for (i = 0; i < 2; i++){		/* Wait for a while so Officer can change availability status */
				Yield();
				Yield();
			}
		}
		printf((int)"Screening officer %d directs passenger %d ", sizeof("Screening officer %d directs passenger %d "), Screen[n].number, z);		/* OFFICIAL OUTPUT STATEMENT */
		printf((int)"to security inspector %d\n", sizeof("to security inspector %d\n"), SPInfo[Screen[n].number].SecurityOfficer, 0);
		Signal(ScreenOfficerCV[Screen[n].number], ScreenLocks[Screen[n].number]);		/* Signal Passenger that they should move on */
		Wait(ScreenOfficerCV[Screen[n].number], ScreenLocks[Screen[n].number]);
		Release(ScreenLocks[Screen[n].number]);
	}
}

/*----------------------------------------------------------------------
// Security Officer
//---------------------------------------------------------------------- */
void SecurityOfficer(){
	int number, z, w, x;
	Acquire(Security_ID_Lock);	
	number = Security_ID;
	Security[number].number = number;
	Security_ID++;
	Release(Security_ID_Lock);
	
	Acquire(SecurityAvail);
	SecurityAvailability[number] = true;
	Release(SecurityAvail);
	Security[number].PassedPassengers = 0;
	printf((int)"Started Security %d\n", sizeof("Started Security %d\n"), number, 0);
	while(true){
		Acquire(SecurityLines);
		Acquire(SecurityLocks[number]);
		if (SecurityLine[0] > 0){		/* Always see if Officer has a line of returning passengers from questioning */
			Signal(SecurityLineCV[0], SecurityLocks[number]);
			
		} else {
			Acquire(SecurityAvail);
			SecurityAvailability[number] = true;		/* Set itself to available */
			Release(SecurityAvail);
		}
		Release(SecurityLines); 

		Wait(SecurityOfficerCV[number], SecurityLocks[number]);	/*Wait for a passenger to arrive*/
		Signal(SecurityOfficerCV[number], SecurityLocks[number]);	/*Signal the passenger to come over*/
		Wait(SecurityOfficerCV[number], SecurityLocks[number]);	/*Wait for the passenger to give info*/
		z = SecPInfo[number].passenger;		/* Get passenger name from passenger */
		Security[number].didPassScreening = ScreeningResult[z];		/* get result of passenger screening test from screening officer */
		
		x = rand() % 5;		/* Generate random value for pass/fail */
		Security[number].SecurityPass = true;		/* Default is pass */
		if (x == 0) Security[number].SecurityPass = false;		/* 20% of failure */
		
		if (SecPInfo[number].questioning){		/* Passenger has just returned from questioning */
			Security[number].TotalPass = true;		/* Allow returned passenger to continue to the boarding area */
			Security[number].PassedPassengers += 1;
			Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move onwards */
			printf((int)"Security inspector %d permits returning passenger %d to board\n", sizeof("Security inspector %d permits returning passenger %d to board\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
			Wait(SecurityOfficerCV[number], SecurityLocks[number]);
		}else {		/* Passenger is first time */
			if (!Security[number].didPassScreening || !Security[number].SecurityPass){
				Security[number].TotalPass = false;		/* Passenger only passes if they pass both tests */
				printf((int)"Security inspector %d is suspicious of the passenger %d\n", sizeof("Security inspector %d is suspicious of the passenger %d\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				SecPInfo[number].PassedSecurity = Security[number].TotalPass;
				printf((int)"Security inspector %d asks passenger %d to go for further examination\n", sizeof("Security inspector %d asks passenger %d to go for further examination\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move to questioning */
			}else {
				Security[number].TotalPass = true;
				printf((int)"Security inspector %d is not suspicious of the passenger %d\n", sizeof("Security inspector %d is not suspicious of the passenger %d\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				SecPInfo[number].PassedSecurity = Security[number].TotalPass;
				Security[number].PassedPassengers += 1;
				printf((int)"Security inspector %d allows passenger %d to board\n", sizeof("Security inspector %d allows passenger %d to board\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move onwards */
			}
		}
		Release(SecurityLocks[number]);
		Yield();
	}
}

/*----------------------------------------------------------------------
// Set up
//---------------------------------------------------------------------- */

void setupAirlines(int airlineCount) {
	int i;
	for(i = 0; i < MAX_CIOS*MAX_AIRLINES+MAX_AIRLINES; i++){
		CheckInLine[i] = 0;
	}
	for(i =0; i < MAX_LIAISONS; i++){
		liaisonLine[i] = 0;
	}
	for(i = 0; i < MAX_SCREEN; i++){
		SecurityAvailability[i] = true;
		SecurityLine[i] = 0;
		ScreenLine[i] = 0;
	}

	for (i = 0; i < airlineCount; i++){
		gates[i] = i;
		boardingLounges[i] = 0;
		totalPassengersOfAirline[i] = simNumOfPassengers/simNumOfAirlines;
		aircraftBaggageCount[i] = 0;		/* Number of baggage on a single airline */
		aircraftBaggageWeight[i] = 0;		/* Weight of baggage on a single airline */
		execLineNeedsHelp[i] = true;
	}
	
	if(simNumOfPassengers%simNumOfAirlines > 0){
		totalPassengersOfAirline[0] += simNumOfPassengers%simNumOfAirlines;
	}
	
	for (i = 0; i < airlineCount; i++){
		/* LiaisonSeat[i] = AIRLINE_SEAT; */
		gateLocks[i] = CreateLock("Gate Lock");
		gateLocksCV[i] = CreateCV("Gate CV");
		execLineLocks[i] = CreateLock("Exec Line Lock");
		execLineCV[i] = CreateCV("Exec Line CV");
	}
	
	for (i = 0; i < airlineCount * AIRLINE_SEAT; i++){
		ScreeningResult[i] = true;
	}
}

void createPassengers(int quantity) {
	int i;

	for(i = 0; i < quantity; i++) {
		simPassengers[i].NotTerrorist = true;
		simPassengers[i].gate = -1;
		simPassengers[i].name = i;
		simPassengers[i].seat = -1;
		simPassengers[i].airline = -1;
		simPassengers[i].myLine = -1;
		simPassengers[i].baggageCount = 2;
		simPassengers[i].bags[1].weight = rand() % 31 + 30;
		simPassengers[i].bags[0].weight = rand() % 31 + 30;
		simPassengers[i].bags[1].airline = -1;
		simPassengers[i].bags[0].airline = -1;
		
		simPassengers[i].economy = true;				/* Default is economy class */
		if(rand() % 3 == 1){		/* 25% of being executive class */
			simPassengers[i].economy = false;
		}
	}
}

void createLiaisons(int quantity) {
	int i;

	for(i = 0; i < quantity; i++) {
		liaisonLine[i] = 0;
		liaisonLineCV[i] = CreateCV("Liaison Line CV " + i);
		liaisonOfficerCV[i] = CreateCV("Liaison Officer CV " + 1);
		liaisonLineLocks[i] = CreateLock("Liaison Line Lock " + i);
		liaisonOfficers[i].airline = -1;
		liaisonOfficers[i].number = i;
		liaisonOfficers[i].passengerCount = 0;
		liaisonOfficers[i].airlineBaggageCount[0] = 0;
		liaisonOfficers[i].airlineBaggageCount[1] = 0;
		liaisonOfficers[i].airlineBaggageCount[2] = 0;
	}
}

void setupEconomyCIOs(int airlineCount, int quantity) {
	int i;
	for (i = 0; i < simNumOfAirlines*simNumOfCIOs; i++){
		CheckInLine[i] = 0;
		CheckInLocks[i] = CreateLock("CheckIn Officer Lock");
		CheckInBreakCV[i] = CreateCV("CheckIn Break Time CV");
		CheckInCV[i] = CreateCV("CheckIn Line CV");
		CheckInOfficerCV[i] = CreateCV("CheckIn Officer CV");
	}
}

void setupExecutiveCIOs(int airlineCount, int quantity) {
	int i;
	for(i = simNumOfAirlines*simNumOfCIOs; i < simNumOfAirlines*simNumOfCIOs + simNumOfAirlines; i++) {
		CheckInLine[i] = 0;
		CheckInLocks[i] = CreateLock("CheckIn Officer Lock");
		CheckInCV[i] = CreateCV("CheckIn Line CV");
	}
}

void createCIOs(int airlineCount, int quantity) {
	int i;
	for(i = 0; i < simNumOfAirlines*simNumOfCIOs; i++) {
		CheckIn[i].airline = i/simNumOfCIOs;
		CheckIn[i].passengerCount = 0;		/* Passenger Count */
		CheckIn[i].OnBreak = false;		/* Controls break time */
		CheckIn[i].work = true;
		CheckIn[i].number = i;
	}
}

void createAirportManager() {
	AirportManager_t simAirportManager;
}

void createSecurityAndScreen(int quantity) {
	int i;
	for (i = 0; i < quantity; i++){
		SecurityLine[i] = 0;
		Security[i].PassedPassengers = 0;
		Security[i].didPassScreening = false;
		Security[i].SecurityPass = false;
		Security[i].TotalPass = false;
		Security[i].number = i;
		Screen[i].IsBusy = false;
		Screen[i].number = i;
		Screen[i].ScreenPass = false;
		ScreenOfficerCV[i] = CreateCV("Screen Officer CV");
		SecurityOfficerCV[i] = CreateCV("Security Officer CV");
		ScreenLocks[i] = CreateLock("Screen Lock");
		SecurityLocks[i] = CreateLock("Security Lock");
		SecurityLineCV[i] = CreateCV("Security line CV");
		simScreeningOfficers[i] = Screen[i];
		simSecurityOfficers[i] = Security[i];
	}
	
	for (i = 0; i < quantity;i++){
		SecurityAvailability[i] = true;
	}
	
	ScreenLineCV[0] = CreateCV("Screen Line CV");
}

void createCargoHandlers(int quantity) {
	int i;
	for(i = 0; i < quantity; i++) {
		cargoHandlers[i].weight[0] = 0;
		cargoHandlers[i].weight[1] = 0;
		cargoHandlers[i].weight[2] = 0;
		cargoHandlers[i].count[0] = 0;
		cargoHandlers[i].count[1] = 0;
		cargoHandlers[i].count[1] = 0;
		cargoHandlers[i].name = i;
		cargoHandlers[i].onBreak = false;
	}
}

void setupBaggageAndCargo(int airlineCount) {
	int i;
	for (i = 0; i < airlineCount; i++){
		AirlineBaggage[i] = CreateLock("Airline Baggage Lock");
	}
	CargoHandlerLock = CreateLock("Cargo Handler Lock");
	CargoHandlerCV = CreateCV("Cargo Handler CV ");
	for(i = 0; i < PASSENGER_COUNT*2; i++){
		conveyor[i].weight = 0;
		conveyor[i].airline = -1;  
	}
}

void setupSingularLocks() {
	Passenger_ID_Lock = CreateLock("");
	Liaison_ID_Lock = CreateLock("");
	CheckIn_ID_Lock = CreateLock("");
	Screening_ID_Lock = CreateLock("");
	Security_ID_Lock = CreateLock("");
	Cargo_ID_Lock = CreateLock("");
	liaisonLineLock = CreateLock("Liaison Line Lock");
	CheckInLock = CreateLock("CheckIn Line Lock");
	ScreenLines = CreateLock("Screen Line Lock");
	airlineSeatLock = CreateLock("Airline Seat Lock");
	/* LiaisonSeats = CreateLock("Liaison Seat Lock"); */
	seatLock = CreateLock("Seat Lock");
	BaggageLock = CreateLock("Baggage Lock");
	SecurityAvail = CreateLock("Security Availability lock");
	SecurityLines = CreateLock("Security Line Lock");
}

void RunSim(){		
	int i;
	simNumOfPassengers = PASSENGER_COUNT;
	simNumOfCargoHandlers = MAX_CARGOHANDLERS;
	simNumOfAirlines = MAX_AIRLINES;
	simNumOfCIOs = MAX_CIOS;
	simNumOfLiaisons = MAX_LIAISONS;
	simNumOfScreeningOfficers = MAX_SCREEN;
	
	for(i = 0; i < simNumOfAirlines; i++) {
		liaisonBaggageCount[i] = 0;
		ticketsIssued[i] = 0;
		alreadyBoarded[i] = false;
	}
	
	createAirportManager();
	setupAirlines(simNumOfAirlines);
	setupSingularLocks();
	setupEconomyCIOs(simNumOfAirlines, simNumOfCIOs);
	setupExecutiveCIOs(simNumOfAirlines, simNumOfCIOs);
	createCIOs(simNumOfAirlines, simNumOfCIOs);
	createLiaisons(simNumOfLiaisons);
	createSecurityAndScreen(simNumOfScreeningOfficers);
	setupBaggageAndCargo(simNumOfAirlines);
	createCargoHandlers(simNumOfCargoHandlers);
	createPassengers(simNumOfPassengers);
	printf((int)"Created everything\n", sizeof("Created everything\n"), 0, 0);
}

void main() {
	int i;
	
	RunSim();	/* Sets up CVs and Locks */

	for(i = 0; i < simNumOfAirlines*simNumOfCIOs; i++) {
		Fork(CheckInOfficer);
	}
	
	for(i = 0; i < simNumOfLiaisons; i++) {
		Fork(Liaison);
	}
	
	for(i = 0; i < simNumOfScreeningOfficers; i++) {
		Fork(ScreeningOfficer);
		Fork(SecurityOfficer);
	}
	
	for(i = 0; i < simNumOfCargoHandlers; i++) {
		Fork(Cargo);
	}
	
	Fork(AirportManager);
	
	for(i = 0; i < simNumOfPassengers; i++) {
		Fork(Passenger);
	}
	
	printf((int)"FORK YOU\n", sizeof("FORK YOU\n"), 0, 0);
}
