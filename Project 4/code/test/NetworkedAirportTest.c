#include "syscall.h"
#include "setup.h"

int main() {
	int i;
	Initialize();
	Write("Execing Airport\n", sizeof("Execing Airport\n"), ConsoleOutput);
	
		Exec("../test/LiaisonOfficer", sizeof("../test/LiaisonOfficer"));
		Exec("../test/LiaisonOfficer", sizeof("../test/LiaisonOfficer"));
		Write("l\n", sizeof("l\n"), ConsoleOutput);
		Exec("../test/CargoHandler", sizeof("../test/CargoHandler"));
		Exec("../test/CargoHandler", sizeof("../test/CargoHandler"));
		Write("ch\n", sizeof("ch\n"), ConsoleOutput);
		Exec("../test/CheckInOfficer", sizeof("../test/CheckInOfficer"));
		Exec("../test/CheckInOfficer", sizeof("../test/CheckInOfficer"));
		Exec("../test/CheckInOfficer", sizeof("../test/CheckInOfficer"));
		Write("cio\n", sizeof("cio\n"), ConsoleOutput);
		Exec("../test/ScreeningOfficer", sizeof("../test/ScreeningOfficer"));
		Exec("../test/ScreeningOfficer", sizeof("../test/ScreeningOfficer"));
		Write("s\n", sizeof("s\n"), ConsoleOutput);
		Exec("../test/SecurityOfficer", sizeof("../test/SecurityOfficer"));
		Exec("../test/SecurityOfficer", sizeof("../test/SecurityOfficer"));
		Write("s\n", sizeof("s\n"), ConsoleOutput);
		Exec("../test/Passenger", sizeof("../test/Passenger"));
		Exec("../test/Passenger", sizeof("../test/Passenger"));
		Exec("../test/Passenger", sizeof("../test/Passenger"));
		Exec("../test/Passenger", sizeof("../test/Passenger"));
		Exec("../test/Passenger", sizeof("../test/Passenger"));
		Exec("../test/Passenger", sizeof("../test/Passenger"));
		Write("p\n", sizeof("p\n"), ConsoleOutput);

	/* for(i = 0; 0 < MAX_CARGOHANDLERS; i++){
		Exec("../test/CargoHandler", sizeof("../test/CargoHandler"));
		Write("ch\n", sizeof("ch\n"), ConsoleOutput);
	}
	for(i = 0; 0 < MAX_CIOS; i++){
		Exec("../test/CheckInOfficer", sizeof("../test/CheckInOfficer"));
		Write("cio\n", sizeof("cio\n"), ConsoleOutput);
	}
	for(i = 0; 0 < MAX_SCREEN; i++){
		Exec("../test/ScreeningOfficer", sizeof("../test/ScreeningOfficer"));
		Write("s\n", sizeof("s\n"), ConsoleOutput);
	}
	for(i = 0; 0 < MAX_SCREEN; i++){
		Exec("../test/SecurityOfficer", sizeof("../test/SecurityOfficer"));
		Write("s\n", sizeof("s\n"), ConsoleOutput);
	}
	for(i = 0; 0 < PASSENGER_COUNT; i++){
		Exec("../test/Passenger", sizeof("../test/Passenger"));
		Write("p\n", sizeof("p\n"), ConsoleOutput);
	} */
	Exec("../test/AirportManager", sizeof("../test/AirportManager"));
	Write("am\n", sizeof("am\n"), ConsoleOutput);
	
	Exit(0);
}