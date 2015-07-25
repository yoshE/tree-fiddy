#include "setup.h"

/*----------------------------------------------------------------------
// CargoHandler
//---------------------------------------------------------------------- */
int main() {
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