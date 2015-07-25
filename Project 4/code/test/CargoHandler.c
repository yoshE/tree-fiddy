#include "setup.h"

/*----------------------------------------------------------------------
// CargoHandler
//---------------------------------------------------------------------- */
int main() {
	int i, n;
	Baggage_t temp;		/* Baggage that handler will move off conveyor */
	Initialize();
	Acquire(Cargo_ID_Lock);	
	n = GetMV(Cargo_ID);
	SetMV(cargoHandlers[n].name, n);
	SetMV(Cargo_ID, n + 1);
	Release(Cargo_ID_Lock);
	printf((int)"Started CARGO %d\n", sizeof("Started CARGO %d\n"), n, 0);
	while (true){
		temp.weight = 0;
		SetMV(cargoHandlers[n].onBreak, false);
		Acquire(CargoHandlerLock);
		for (i = 0; i < PASSENGER_COUNT*2; i++){
			if(GetMV(conveyor[i].weight) != 0){
				temp.weight = GetMV(conveyor[i].weight);
				temp.airline = GetMV(conveyor[i].airline);
				SetMV(conveyor[i].weight, 0);
				break;
			}
		}
		if (temp.weight == 0){
			SetMV(cargoHandlers[n].onBreak, true);
			printf((int)"Cargo Handler %d is going for a break\n", sizeof("Cargo Handler %d is going for a break\n"), n, -1);		/* OFFICIAL OUTPUT STATEMENT */
			Wait(CargoHandlerCV, CargoHandlerLock);		/* Sleep until woken up by manager */
			Release(CargoHandlerLock);
			printf((int)"Cargo Handler %d returned from break\n", sizeof("Cargo Handler %d returned from break\n"), n, -1);		/* OFFICIAL OUTPUT STATEMENT */
			continue;
		}
	
		Acquire(AirlineBaggage[temp.airline]);		/* Acquire lock to put baggage on an airline */
		Release(CargoHandlerLock);
		SetMV(aircraftBaggageCount[temp.airline], aircraftBaggageCount[temp.airline] + 1);		/* Increase baggage count of airline */
		SetMV(aircraftBaggageWeight[temp.airline], aircraftBaggageWeight[temp.airline] + temp.weight);		/* Increase baggage weight of airline */
		printf((int)"Cargo Handler %d picked bag of airline %d ", sizeof("Cargo Handler %d picked bag of airline %d "), n, temp.airline);		/* OFFICIAL OUTPUT STATEMENT */
		printf((int)"with weighing %d lbs\n", sizeof("with weighing %d lbs\n"), temp.weight, 0);
		printf((int)"ID is: %d\n", sizeof("ID is: %d\n"), Cargo_ID, -1);
		printf((int)"number is: %d\n", sizeof("number is: %d\n"), n, -1);
		SetMV(cargoHandlers[n].weight[temp.airline], cargoHandlers[n].weight[temp.airline] + temp.weight);		/* Increment total weight of baggage this handler has dealt with */
		SetMV(cargoHandlers[n].count[temp.airline], cargoHandlers[n].count[temp.airline] + 1);		/* Increment total count of baggage this handler has dealt with */
		Release(AirlineBaggage[temp.airline]);
		temp.weight = 0;
		Yield();
	}
}