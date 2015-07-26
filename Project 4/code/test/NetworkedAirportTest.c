#include "syscall.h"
#include "setup.h"

int main() {
	int i;
	Write("Execing Airport\n", sizeof("Execing Airport\n"), ConsoleOutput);
	
	for(i = 0; 0 < MAX_LIAISONS; i++){
		Exec("../test/LiaisonOfficer", sizeof("../test/LiaisonOfficer"));
	}
	for(i = 0; 0 < MAX_CARGOHANDLERS; i++){
		Exec("../test/CargoHandler", sizeof("../test/CargoHandler"));
	}
	for(i = 0; 0 < MAX_CIOS; i++){
		Exec("../test/CheckInOfficer", sizeof("../test/CheckInOfficer"));
	}
	for(i = 0; 0 < MAX_SCREEN; i++){
		Exec("../test/ScreeningOfficer", sizeof("../test/ScreeningOfficer"));
	}
	for(i = 0; 0 < MAX_SCREEN; i++){
		Exec("../test/SecurityOfficer", sizeof("../test/SecurityOfficer"));
	}
	for(i = 0; 0 < PASSENGER_COUNT; i++){
		Exec("../test/Passenger", sizeof("../test/Passenger"));
	}
	Exec("../test/AirportManager", sizeof("../test/AirportManager"));
	
	Exit(0);
}