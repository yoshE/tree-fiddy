#include "setup.h"

/*----------------------------------------------------------------------
// LiaisonOfficer
//---------------------------------------------------------------------- */

int Liaison_getPassengerCount(int n) {
	return liaisonOfficers[n].passengerCount;
} /* For manager to get passenger headcount */

int Liaison_getAirlineBaggageCount(int n) {
	/* For manager to get passenger bag count */
	return liaisonOfficers[n].airlineBaggageCount[n];
}

int main() {
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