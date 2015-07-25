#include "setup.h"

/*----------------------------------------------------------------------
// Passenger
//---------------------------------------------------------------------- */

int main() {		
	/* Picks a Liaison line, talks to the Officer, gets airline */
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
		} else {		/* Otherwise just add yourself to the line and wait (Security Officer will not take anymore passengers as you are in queue) */
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