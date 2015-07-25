#include "setup.h"

/*----------------------------------------------------------------------
// Airport Manager
//---------------------------------------------------------------------- */

int main() {
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