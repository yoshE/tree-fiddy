#include "setup.h"

/*----------------------------------------------------------------------
// Airport Manager
//---------------------------------------------------------------------- */

int main() {
	int i;
	printf((int)"TESTY FACK AM\n", sizeof("TESTY FACK AM\n"), 0, 0);
	Initialize();
	for(i = 0; i < MAX_AIRLINES; i++){
		simAirportManager.CargoHandlerTotalWeight[i] = CreateMV("CargoHandlerTotalWeight ", 0, i);
		simAirportManager.CargoHandlerTotalCount[i] = CreateMV("CargoHandlerTotalCount ", 0, i);
		simAirportManager.CIOTotalCount[i] = CreateMV("CIOTotalCount ", 0, i);
		simAirportManager.CIOTotalWeight[i] = CreateMV("CIOTotalWeight ", 0, i);
		simAirportManager.LiaisonTotalCount[i] = CreateMV("LiaisonTotalCount ", 0, i);
		simAirportManager.liaisonPassengerCount = CreateMV("liaisonPassengerCount ", 0, i);
		simAirportManager.checkInPassengerCount = CreateMV("checkInPassengerCount ", 0, i);
		simAirportManager.securityPassengerCount = CreateMV("securityPassengerCount ", 0, i);
	}
	Manager_DoWork();
}

void EndOfDay(){
	int i, j;
	for(i = 0; i < simNumOfCargoHandlers; i++){
		for(j = 0; j < simNumOfAirlines; j++){
			SetMV(simAirportManager.CargoHandlerTotalWeight[j], simAirportManager.CargoHandlerTotalWeight[j] + GetMV(cargoHandlers[i].weight[j]));
			SetMV(simAirportManager.CargoHandlerTotalCount[j], simAirportManager.CargoHandlerTotalCount[j] + GetMV(cargoHandlers[i].count[j]));
		}
	}
	for(i = 0; i < simNumOfCIOs * simNumOfAirlines; i++){
		for(j = 0; j < PASSENGER_COUNT*2; j++){
			SetMV(simAirportManager.CIOTotalWeight[CheckIn[i].totalBags[j].airline], simAirportManager.CIOTotalWeight[CheckIn[i].totalBags[j].airline] + GetMV(CheckIn[i].totalBags[j].weight));
		}
		SetMV(simAirportManager.checkInPassengerCount, simAirportManager.checkInPassengerCount + GetMV(CheckIn[i].passengerCount));
	}

	for(i = 0; i < simNumOfLiaisons; i++){
		for(j = 0; j < simNumOfAirlines; j++){
			SetMV(simAirportManager.LiaisonTotalCount[j], simAirportManager.LiaisonTotalCount[j] + GetMV(liaisonOfficers[i].airlineBaggageCount[j]));
		}
		SetMV(simAirportManager.liaisonPassengerCount, simAirportManager.liaisonPassengerCount + GetMV(liaisonOfficers[i].passengerCount));
	}
	
	for(i = 0; i < simNumOfScreeningOfficers; i++){
		SetMV(simAirportManager.securityPassengerCount, simAirportManager.securityPassengerCount + GetMV(Security[i].PassedPassengers));
	}
	printf((int)"Passenger count reported by airport liaison = %d\n", sizeof("Passenger count reported by airport liaison = %d\n"), GetMV(simAirportManager.liaisonPassengerCount), -1);
	printf((int)"Passenger count reported by airport liaison = %d\n", sizeof("Passenger count reported by airport liaison = %d\n"), GetMV(simAirportManager.checkInPassengerCount), -1);
	printf((int)"Passenger count reported by security inspector = %d\n", sizeof("Passenger count reported by security inspector = %d\n"), GetMV(simAirportManager.securityPassengerCount), -1);
	
	for(i = 0; i < simNumOfAirlines; i++){
		printf((int)"From setup: Baggage count of airline %d = %d\n", sizeof("From setup: Baggage count of airline %s = %d\n"), i, GetMV(totalBaggage[i]));
		printf((int)"From airport liaison: Baggage count of airline %d = %d\n", sizeof("From airport liaison: Baggage count of airline %d = %d\n"), i, GetMV(simAirportManager.LiaisonTotalCount[i]));
		printf((int)"From cargo handlers: Baggage count of airline %d = %d\n", sizeof("From cargo handlers: Baggage count of airline %d = %d\n"), i, GetMV(simAirportManager.CargoHandlerTotalCount[i]));
		printf((int)"From setup: Baggage weight of airline %d = %d\n", sizeof("From setup: Baggage weight of airline %d = %d\n"), i, GetMV(totalWeight[i]));
		printf((int)"From airline check-in staff: Baggage weight of airline %d = %d\n", sizeof("From airline check-in staff: Baggage weight of airline %d = %d\n"), i, GetMV(simAirportManager.CIOTotalWeight[i]));
		printf((int)"From cargo handlers: Baggage weight of airline %d = %d\n", sizeof("From cargo handlers: Baggage weight of airline %d = %d\n"), i, GetMV(simAirportManager.CargoHandlerTotalWeight[i]));
	}
}

void Manager_DoWork(){
	int breakCount, y;
	while(true){
		int i;
		for (i = 0; i < PASSENGER_COUNT*2; i++){
			if(GetMV(conveyor[i].weight) != 0){
				Acquire(CargoHandlerLock);
				breakCount = 0;
				for(y = 0; y < simNumOfCargoHandlers; y++){
				printf((int)"CH %d on break? %d\n", sizeof("CH %d on break? %d\n"), y, GetMV(cargoHandlers[y].onBreak));
					if(GetMV(cargoHandlers[y].onBreak) == 1){
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
			if(GetMV(boardingLounges[i]) == GetMV(totalPassengersOfAirline[i]) && GetMV(totalBaggage[i]) == GetMV(aircraftBaggageCount[i]) && !GetMV(alreadyBoarded[i])){
				Acquire(gateLocks[i]);
				Broadcast(gateLocksCV[i], gateLocks[i]);
				SetMV(alreadyBoarded[i], true);
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
