#include "setup.h"

/*----------------------------------------------------------------------
// LiaisonOfficer
//---------------------------------------------------------------------- */

int Liaison_getPassengerCount(int n) {
	return GetMV(liaisonOfficers[n].passengerCount);
} /* For manager to get passenger headcount */

int Liaison_getAirlineBaggageCount(int n) {
	/* For manager to get passenger bag count */
	return GetMV(liaisonOfficers[n].airlineBaggageCount[n]);
}

int main() {
	int name;
	printf((int)"TESTY FACK L\n", sizeof("TESTY FACK U\n"), 0, 0);
	Initialize();
	Acquire(Liaison_ID_Lock);
	name = GetMV(Liaison_ID);
	SetMV(liaisonOfficers[name].number, name);
	SetMV(Liaison_ID, Liaison_ID + 1);
	Release(Liaison_ID_Lock);
	printf((int)"Started Liaison %d\n", sizeof("Started Liaison %d\n"), name, 0);

	while(true){		/* Always be running, never go on break */
			/* printf((int)"LIAI %d\n", sizeof("LIAI %d\n"), name, 0); */

		Acquire(liaisonLineLock);		/* Acquire lock for lining up in order to see if there is someone waiting in your line */
		if (GetMV(liaisonLine[name]) > 0){		/* Check if passengers are in your line */
		printf((int)"LiaiLine %d = %d\n", sizeof("LiaiLine %d = %d\n"), name, GetMV(liaisonLine[name]));
			Signal(liaisonLineCV[name], liaisonLineLock);		/* Signal them if there are */
			Acquire(liaisonLineLocks[name]);		
			Release(liaisonLineLock);
			printf((int)"LIAI ACQUIRED %d\n", sizeof("LIAI ACQUIRED %d\n"), liaisonLineLocks[name], 0);
			
			Wait(liaisonOfficerCV[name], liaisonLineLocks[name]);		/* Wait for passenger to give you baggage info */
			
			printf((int)"GOT BAGGAGE INFO LIAI %d\n", sizeof("GOT BAGGAGE INFO LIAI %d\n"), name, 0);
			
			/* Passenger has given bag Count info and woken up the Liaison Officer */
			SetMV(liaisonOfficers[name].passengerCount, liaisonOfficers[name].passengerCount + 1);		/*Increment internal passenger counter */
			Acquire(seatLock);
			while(true){
				SetMV(liaisonOfficers[name].airline, rand() % simNumOfAirlines);
				if (GetMV(ticketsIssued[liaisonOfficers[name].airline]) < GetMV(totalPassengersOfAirline[liaisonOfficers[name].airline])){
					SetMV(ticketsIssued[liaisonOfficers[name].airline], ticketsIssued[liaisonOfficers[name].airline] + 1);
					break;
				}
			}
			SetMV(liaisonOfficers[name].airlineBaggageCount[liaisonOfficers[name].airline], liaisonOfficers[name].airlineBaggageCount[liaisonOfficers[name].airline] + LPInfo[liaisonOfficers[name].number].baggageCount);
			Release(seatLock);
			SetMV(LPInfo[liaisonOfficers[name].number].airline, liaisonOfficers[name].airline);		/* Put airline number in shared struct for passenger */
			SetMV(liaisonBaggageCount[liaisonOfficers[name].airline], liaisonBaggageCount[liaisonOfficers[name].airline] + LPInfo[liaisonOfficers[name].number].baggageCount);
			Signal(liaisonOfficerCV[name], liaisonLineLocks[name]); /* Wakes up passenger */
			Wait(liaisonOfficerCV[name], liaisonLineLocks[name]); /* Waits for Passenger to say they are leaving */
			printf((int)"Airport Liaison %d directed passenger %d ", sizeof("Airport Liaison %d directed passenger %d "), name, GetMV(LPInfo[name].passengerName));		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"of airline %d\n", sizeof("of airline %d\n"), GetMV(liaisonOfficers[name].airline),0);
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