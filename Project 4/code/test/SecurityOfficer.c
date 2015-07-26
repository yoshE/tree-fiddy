#include "setup.h"
#include "syscall.h"

/*----------------------------------------------------------------------
// Security Officer
//---------------------------------------------------------------------- */
int main(){
	int number, z, w, x;
	Initialize();
	Acquire(Security_ID_Lock);	
	number = GetMV(Security_ID);
	SetMV(Security_ID, number + 1);
	Release(Security_ID_Lock);
	
	Acquire(SecurityAvail);
	SetMV(SecurityAvailability[number], 1);
	Release(SecurityAvail);
	
	while(true){
		Acquire(SecurityLines);
		Acquire(SecurityLocks[number]);
		if (GetMV(SecurityLine[0]) > 0){		/* Always see if Officer has a line of returning passengers from questioning */
			Signal(SecurityLineCV[0], SecurityLocks[number]);
			
		} else {
			Acquire(SecurityAvail);
			SetMV(SecurityAvailability[number], 1);		/* Set itself to available */
			Release(SecurityAvail);
		}
		Release(SecurityLines); 
		Wait(SecurityOfficerCV[number], SecurityLocks[number]);	/*Wait for a passenger to arrive*/
		Signal(SecurityOfficerCV[number], SecurityLocks[number]);	/*Signal the passenger to come over*/
		Wait(SecurityOfficerCV[number], SecurityLocks[number]);	/*Wait for the passenger to give info*/
		z = GetMV(SecPInfo[number].passenger);		/* Get passenger name from passenger */
		SetMV(Security[number].didPassScreening, GetMV(ScreeningResult[z]));		/* get result of passenger screening test from screening officer */
		
		x = rand() % 5;		/* Generate random value for pass/fail */
		SetMV(Security[number].SecurityPass, 1);		/* Default is pass */
		if (x == 0) SetMV(Security[number].SecurityPass, 0);		/* 20% of failure */
		
		if (GetMV(SecPInfo[number].questioning)){		/* Passenger has just returned from questioning */
			SetMV(Security[number].TotalPass, 1);		/* Allow returned passenger to continue to the boarding area */
			SetMV(Security[number].PassedPassengers, GetMV(Security[number].PassedPassengers + 1));
			
			Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move onwards */
			printf((int)"Security inspector %d permits returning passenger %d to board\n", sizeof("Security inspector %d permits returning passenger %d to board\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
			Wait(SecurityOfficerCV[number], SecurityLocks[number]);
		}else {		/* Passenger is first time */
			if (!GetMV(Security[number].didPassScreening) || !GetMV(Security[number].SecurityPass)){
				SetMV(Security[number].TotalPass, 0);		/* Passenger only passes if they pass both tests */
				printf((int)"Security inspector %d is suspicious of the passenger %d\n", sizeof("Security inspector %d is suspicious of the passenger %d\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				
				SetMV(SecPInfo[number].PassedSecurity, GetMV(Security[number].TotalPass));
				printf((int)"Security inspector %d asks passenger %d to go for further examination\n", sizeof("Security inspector %d asks passenger %d to go for further examination\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move to questioning */
			}else {
				SetMV(Security[number].TotalPass, 1);
				printf((int)"Security inspector %d is not suspicious of the passenger %d\n", sizeof("Security inspector %d is not suspicious of the passenger %d\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				
				SetMV(SecPInfo[number].PassedSecurity, GetMV(Security[number].TotalPass));
				SetMV(Security[number].PassedPassengers, GetMV(Security[number].PassedPassengers + 1));
				printf((int)"Security inspector %d allows passenger %d to board\n", sizeof("Security inspector %d allows passenger %d to board\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move onwards */
			}
		}
		Release(SecurityLocks[number]);
		Yield();
	}
}