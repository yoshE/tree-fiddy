#include "setup.h"

/*----------------------------------------------------------------------
// Security Officer
//---------------------------------------------------------------------- */
void SecurityOfficer(){
	int number, z, w, x;
	Initialize();
	Acquire(Security_ID_Lock);	
	number = Security_ID;
	Security[number].number = number;
	Security_ID++;
	Release(Security_ID_Lock);
	
	Acquire(SecurityAvail);
	SecurityAvailability[number] = true;
	Release(SecurityAvail);
	Security[number].PassedPassengers = 0;
	printf((int)"Started Security %d\n", sizeof("Started Security %d\n"), number, 0);
	while(true){
		Acquire(SecurityLines);
		Acquire(SecurityLocks[number]);
		if (SecurityLine[0] > 0){		/* Always see if Officer has a line of returning passengers from questioning */
			Signal(SecurityLineCV[0], SecurityLocks[number]);
			
		} else {
			Acquire(SecurityAvail);
			SecurityAvailability[number] = true;		/* Set itself to available */
			Release(SecurityAvail);
		}
		Release(SecurityLines); 

		Wait(SecurityOfficerCV[number], SecurityLocks[number]);	/*Wait for a passenger to arrive*/
		Signal(SecurityOfficerCV[number], SecurityLocks[number]);	/*Signal the passenger to come over*/
		Wait(SecurityOfficerCV[number], SecurityLocks[number]);	/*Wait for the passenger to give info*/
		z = SecPInfo[number].passenger;		/* Get passenger name from passenger */
		Security[number].didPassScreening = ScreeningResult[z];		/* get result of passenger screening test from screening officer */
		
		x = rand() % 5;		/* Generate random value for pass/fail */
		Security[number].SecurityPass = true;		/* Default is pass */
		if (x == 0) Security[number].SecurityPass = false;		/* 20% of failure */
		
		if (SecPInfo[number].questioning){		/* Passenger has just returned from questioning */
			Security[number].TotalPass = true;		/* Allow returned passenger to continue to the boarding area */
			Security[number].PassedPassengers += 1;
			Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move onwards */
			printf((int)"Security inspector %d permits returning passenger %d to board\n", sizeof("Security inspector %d permits returning passenger %d to board\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
			Wait(SecurityOfficerCV[number], SecurityLocks[number]);
		}else {		/* Passenger is first time */
			if (!Security[number].didPassScreening || !Security[number].SecurityPass){
				Security[number].TotalPass = false;		/* Passenger only passes if they pass both tests */
				printf((int)"Security inspector %d is suspicious of the passenger %d\n", sizeof("Security inspector %d is suspicious of the passenger %d\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				SecPInfo[number].PassedSecurity = Security[number].TotalPass;
				printf((int)"Security inspector %d asks passenger %d to go for further examination\n", sizeof("Security inspector %d asks passenger %d to go for further examination\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move to questioning */
			}else {
				Security[number].TotalPass = true;
				printf((int)"Security inspector %d is not suspicious of the passenger %d\n", sizeof("Security inspector %d is not suspicious of the passenger %d\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				SecPInfo[number].PassedSecurity = Security[number].TotalPass;
				Security[number].PassedPassengers += 1;
				printf((int)"Security inspector %d allows passenger %d to board\n", sizeof("Security inspector %d allows passenger %d to board\n"), number, z);		/* OFFICIAL OUTPUT STATEMENT */
				Signal(SecurityOfficerCV[number], SecurityLocks[number]);		/* Signal passenger to move onwards */
			}
		}
		Release(SecurityLocks[number]);
		Yield();
	}
}