#include "setup.h"

/*----------------------------------------------------------------------
// CheckInOfficer
//---------------------------------------------------------------------- */
int main() {
	int i;
	Initialize();
	Acquire(CheckIn_ID_Lock);	
	i = GetMV(CheckIn_ID);
	SetMV(CheckIn[i].number, i);
	SetMV(CheckIn_ID, i + 1);
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
		x = simNumOfAirlines*simNumOfCIOs + GetMV(CheckIn[number].airline);		/* Check Executive Line for your airline first */
		if (GetMV(CheckIn[number].OnBreak) == 1) SetMV(CheckIn[number].OnBreak, false);
		/* execLineLocks[info.airline]->Acquire(); */
		if(GetMV(CheckInLine[x]) > 0 && GetMV(execLineNeedsHelp[CheckIn[number].airline])){
			SetMV(execLineNeedsHelp[CheckIn[number].airline], false);
			helpedExecLine = true;
			SetMV(CPInfo[x].line, CheckIn[number].number);
			SetMV(CheckInLine[x], CheckInLine[x] - 1);
			Signal(CheckInCV[x], CheckInLock);
			printf((int)"Airline check-in staff %d of airline %d serves an executive class passenger and economy class line length = ", sizeof("Airline check-in staff %d of airline %d serves an executive class passenger and economy class line length = "), number, GetMV(CheckIn[number].airline));		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"%d\n", sizeof("%d\n"), CheckInLine[CheckIn[number].number], 0);
			SetMV(CheckInLine[CheckIn[number].number], CheckInLine[CheckIn[number].number] + 1);
			/* execLineCV[info.airline]->Wait(execLineLocks[info.airline]); */
		} else if (CheckInLine[CheckIn[number].number] > 0){		/* If no executive, then check your normal line */
			Signal(CheckInCV[CheckIn[number].number], CheckInLock);
			printf((int)"Airline check-in staff %d of airline %d serves an economy class passenger and executive class line length = ", sizeof("Airline check-in staff %d of airline %d serves an economy class passenger and executive class line length = "), number, GetMV(CheckIn[number].airline));		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"%d\n", sizeof("%d\n"), CheckInLine[x], 0);
		}else {		/* Else, there are no passengers waiting and you can go on break */
			/* if(execLineLocks[info.airline]->isHeldByCurrentThread()){
				// execLineLocks[info.airline]->Release();
			// } */
			SetMV(CheckIn[number].OnBreak, true);
			Wait(CheckInBreakCV[CheckIn[number].number], CheckInLock);
			Release(CheckInLock);
			continue;
		}
		/* if(execLineLocks[info.airline]->isHeldByCurrentThread()){
			// execLineLocks[info.airline]->Release();
		// } */
		SetMV(CheckIn[number].passengerCount, CheckIn[number].passengerCount + 1);
		Acquire(CheckInLocks[CheckIn[number].number]);
		Release(CheckInLock);
		Wait(CheckInOfficerCV[CheckIn[number].number], CheckInLocks[CheckIn[number].number]);		/* Wait for passenger to give you baggage and airline */
		if(helpedExecLine){
			SetMV(execLineNeedsHelp[CheckIn[number].airline], true);
		}
		for(i = 0; i < GetMV(CPInfo[CheckIn[number].number].baggageCount); i++){
			if(GetMV(CPInfo[CheckIn[number].number].bag[i].weight) != 0){
				SetMV(CheckIn[number].bags[i].weight, CPInfo[CheckIn[number].number].bag[i].weight);		/* Place baggage info in shared struct into internal struct */
				SetMV(CheckIn[number].bags[i].airline, CPInfo[CheckIn[number].number].bag[i].airline);		/* Place baggage info in shared struct into internal struct */
			} else {
				SetMV(CheckIn[number].bags[i].weight, 0);
			}
		}
		
		for (i = 0; i < GetMV(CPInfo[CheckIn[number].number].baggageCount); i++){	
			SetMV(CheckIn[number].bags[i].airline, CheckIn[number].airline);		/* Add airline code to baggage */
			Acquire(BaggageLock);			/* Acquire lock for putting baggage on conveyor */
			
			for(y = 0; y < PASSENGER_COUNT*2; y++){
				if(GetMV(conveyor[y].weight) == 0){
					SetMV(conveyor[y].weight, CheckIn[number].bags[i].weight);
					SetMV(conveyor[y].airline, CheckIn[number].bags[i].airline);
					printf((int)"Airline check-in staff %d of airline %d dropped bags to the conveyor system\n", sizeof("Airline check-in staff %d of airline %d dropped bags to the conveyor system\n"), number, GetMV(CheckIn[number].airline));		/* OFFICIAL OUTPUT STATEMENT */
					break;
				}
			}
			
			/*conveyor.push_back(CheckIn[number].bags[i]);		/* Place baggage onto conveyor belt for Cargo Handlers */
			Release(BaggageLock);		/* Release baggage lock */
			SetMV(CheckIn[number].totalBags[(CheckIn[number].passengerCount - 2) + i].weight, CheckIn[number].bags[i].weight);
			SetMV(CheckIn[number].totalBags[(CheckIn[number].passengerCount - 2) + i].airline, CheckIn[number].bags[i].airline);
		}
		
		SetMV(CPInfo[CheckIn[number].number].gate, gates[CheckIn[number].airline]);		/* Tell Passenger Gate Number*/
		if (!GetMV(CPInfo[CheckIn[number].number].IsEconomy)){
			printf((int)"Airline check-in staff %d of airline %d ", sizeof("Airline check-in staff %d of airline %d "), number, GetMV(CheckIn[number].airline));		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"informs executive class passenger %d to board at gate %d\n", sizeof("informs executive class passenger %d to board at gate %d\n"), GetMV(CPInfo[number].passenger), GetMV(gates[CheckIn[number].airline]));
		} else {
			printf((int)"Airline check-in staff %d of airline %d ", sizeof("Airline check-in staff %d of airline %d "), number, GetMV(CheckIn[number].airline));		/* OFFICIAL OUTPUT STATEMENT */
			printf((int)"informs economy class passenger %d to board at gate %d\n", sizeof("informs economy class passenger %d to board at gate %d\n"), GetMV(CPInfo[number].passenger), GetMV(gates[CheckIn[number].airline]));
		}
		Signal(CheckInOfficerCV[CheckIn[number].number], CheckInLocks[CheckIn[number].number]);
		Wait(CheckInOfficerCV[CheckIn[number].number], CheckInLocks[CheckIn[number].number]);		/* Passenger will wake up you when they leave, starting the loop over again */
		Release(CheckInLocks[CheckIn[number].number]);
		Yield();
	}
	printf((int)"Airline check-in staff %d is closing the counter\n", sizeof("Airline check-in staff %d is closing the counter\n"), GetMV(CheckIn[number].number), -1);		/* OFFICIAL OUTPUT STATEMENT */
}