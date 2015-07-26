#include "setup.h"

/*----------------------------------------------------------------------
// Screening Officer
//---------------------------------------------------------------------- */
void Screening_setBusy(int n){
	Acquire(ScreenLines);
	Screen[n].IsBusy = true;
	Release(ScreenLines);
}

int main(){
	int n;
	printf((int)"TESTY FACK SC\n", sizeof("TESTY FACK UC\n"), 0, 0);
	Initialize();
	Acquire(Screening_ID_Lock);	
	n = GetMV(Screening_ID);
	SetMV(Screen[n].number, n);
	SetMV(Screening_ID, n + 1);
	Release(Screening_ID_Lock);
	
	Acquire(ScreenLines);
	SetMV(Screen[n].IsBusy, false);		/* Set Officer to available */
	Release(ScreenLines);
	
	printf((int)"Started Screening %d\n", sizeof("Started Screening %d\n"), n, 0);
	while(true){
		int i, y, z, x;
		Acquire(ScreenLines);
		if (GetMV(Screen[n].IsBusy)){ 
			SetMV(Screen[n].IsBusy, false);		/* If busy, should no longer be busy */
		}
		if (GetMV(ScreenLine[0])){		/* Checks if the screening line has passengers */
			Signal(ScreenLineCV[0], ScreenLines);		/* Wake them if there are */
		}
		Acquire(ScreenLocks[Screen[n].number]);
		Release(ScreenLines);
		Wait(ScreenOfficerCV[Screen[n].number], ScreenLocks[Screen[n].number]);		/* Wait for Passenger to start conversation */

		z = SPInfo[Screen[n].number].passenger;		/* Find passenger name */
		x = rand() % 5;		/* Generate random value for pass/fail */
		SetMV(Screen[n].ScreenPass, true);		/* Default is pass */
		if (x == 0) SetMV(Screen[n].ScreenPass, false);		/* 20% of failure */
		SetMV(ScreeningResult[z], Screen[n].ScreenPass);
		if (GetMV(Screen[n].ScreenPass)){		/* If passenger passed test */
			printf((int)"Screening officer %d is not suspicious of the hand luggage of passenger %d\n", sizeof("Screening officer %d is not suspicious of the hand luggage of passenger %d\n"), n, z);		/* OFFICIAL OUTPUT STATEMENT */
		}else {
			printf((int)"Screening officer %d is suspicious of the hand luggage of passenger %d\n", sizeof("Screening officer %d is suspicious of the hand luggage of passenger %d\n"), n, z);		/* OFFICIAL OUTPUT STATEMENT */
		}
		while(true){		/* Wait for Security Officer to become available */
			y = false;
			for (i = 0; i < simNumOfScreeningOfficers; i++){		/* Iterate through all security officers */
				Acquire(SecurityAvail);
				y = GetMV(SecurityAvailability[i]);		/* See if they are busy */
				if (y){			/* If a security officer is not busy, obtain his number and inform passenger */
					SetMV(SPInfo[Screen[n].number].SecurityOfficer, i);
					SetMV(SecurityAvailability[i], false);
				}
				Release(SecurityAvail);
			}
			if(y){
				break;
			}
			for (i = 0; i < 2; i++){		/* Wait for a while so Officer can change availability status */
				Yield();
				Yield();
			}
		}
		printf((int)"Screening officer %d directs passenger %d ", sizeof("Screening officer %d directs passenger %d "), GetMV(Screen[n].number), z);		/* OFFICIAL OUTPUT STATEMENT */
		printf((int)"to security inspector %d\n", sizeof("to security inspector %d\n"), GetMV(SPInfo[Screen[n].number].SecurityOfficer), 0);
		Signal(ScreenOfficerCV[Screen[n].number], ScreenLocks[Screen[n].number]);		/* Signal Passenger that they should move on */
		Wait(ScreenOfficerCV[Screen[n].number], ScreenLocks[Screen[n].number]);
		Release(ScreenLocks[Screen[n].number]);
	}
}