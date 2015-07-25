#include "syscall.h"


#define true 1
#define false 0

/* Max agent consts */
#define PASSENGER_COUNT			9
#define MAX_AIRLINES			3
#define MAX_LIAISONS			3
#define MAX_CIOS				3
#define MAX_CARGOHANDLERS		4
#define MAX_SCREEN				3
#define MAX_BAGS				3
#define AIRLINE_SEAT 			3

int Passenger_ID = 0;
int Liaison_ID = 0;
int Screening_ID = 0;
int CheckIn_ID = 0;
int Security_ID = 0;
int Cargo_ID = 0;

int Passenger_ID_Lock;
int Liaison_ID_Lock;
int CheckIn_ID_Lock;
int Screening_ID_Lock;
int Security_ID_Lock;
int Cargo_ID_Lock;

/*----------------------------------------------------------------------
// Arrays, Lists, and Vectors
//---------------------------------------------------------------------- */
int SecurityAvailability[MAX_SCREEN];		/* Array of Bools for availability of each security officer */
int liaisonLine[MAX_LIAISONS];		/* Array of line sizes for each Liaison Officer */
int CheckInLine[MAX_CIOS*MAX_AIRLINES+MAX_AIRLINES];		/* Array of line sizes for each CheckIn Officer */
int SecurityLine[MAX_SCREEN];		/* Array of line sizes for return passengers from security questioning */
int ScreenLine[MAX_SCREEN];		/* Array of line sizes for each Screening Officer */
int ScreenLineCV[MAX_SCREEN];			/* Condition Variables for the Screening Line */
int ScreenOfficerCV[MAX_SCREEN];		/* Condition Variables for each Screening Officer */
int SecurityOfficerCV[MAX_SCREEN];		/* Condition Variables for each Security Officer */
int SecurityLineCV[MAX_SCREEN];		/* Condition Variables for returning passengers from questioning */
int liaisonLineCV[MAX_LIAISONS];		/* Condition Variables for each Liaison Line */
int liaisonOfficerCV[MAX_LIAISONS];		/* Condition Variables for each Liaison Officer */
int CheckInCV[MAX_CIOS*MAX_AIRLINES+MAX_AIRLINES];		/* Condition Variables for each CheckIn Line */
int CheckInOfficerCV[MAX_CIOS*MAX_AIRLINES];		/* Condition Variables for each CheckIn Officer */
int CheckInBreakCV[MAX_CIOS*MAX_AIRLINES];		/* Condition Variables for each CheckIn Officer Break Time */
int liaisonLineLock;		/* Lock to get into a liaison Line */
int liaisonLineLocks[MAX_LIAISONS];		/* Array of Locks for Liaison Officers */
int CheckInLocks[MAX_CIOS*MAX_LIAISONS];		/* Array of Locks for CheckIn Officers */
int ScreenLocks[MAX_SCREEN];		/* Array of Locks for Screening Officers */
int SecurityLocks[MAX_SCREEN];		/* Array of Locks for Security Officers */
int AirlineBaggage[MAX_LIAISONS]; 		/* Array of Locks for placing baggage on airlines */
int CheckInLock;		/* Lock to get into CheckIn Line */
int ScreenLines;		/* Lock to get into Screening Line */
int CargoHandlerLock;		/* Lock for Cargo Handlers for taking baggage off conveyor */
int CargoHandlerCV;		/* Condition Variable for Cargo Handlers */
int airlineSeatLock;		/* Lock for find seat number for customers */
int BaggageLock;		/* Lock for placing Baggage onto the conveyor */
int SecurityAvail;		/* Lock for seeing if a Security Officer is busy */
int SecurityLines;			/* Lock for returning passengers from Security */
int gateLocks[MAX_AIRLINES];				/* Locks for waiting at the gate */
int gateLocksCV[MAX_AIRLINES];		/* CVs for waiting at the gate */
int seatLock;			/* Lock for assigned airline seats in the Liaison */
int execLineLocks[MAX_AIRLINES];
int execLineCV[MAX_AIRLINES];
/*----------------------------------------------------------------------
// Structs
//---------------------------------------------------------------------- */
typedef struct {
	int weight;
	int airline;
}Baggage_t;

typedef struct{
	int NotTerrorist;
	int gate;
	int name;        
	int seat;	
	int airline;		
	int economy;	
	int myLine;		
	int baggageCount;	
	Baggage_t bags[2];
}Passenger_t;

typedef struct{
	int number;
	int passengerCount;
	Baggage_t bags[2];
	Baggage_t totalBags[PASSENGER_COUNT*2];
	int airline;
	int OnBreak;		/* If there are no passengers in line, the officer goes on break until manager makes them up */
	int work;		/* Always working until all passengers have finished checking in */
}CheckInOfficer_t;

typedef struct{
	int airline;		/* Airline the liaison will assign to the passenger */
	int number;		/* Number of the liaison (which line they control) */
	int passengerCount;		/* Number of passengers the liaison has helped */
	int airlineBaggageCount[MAX_AIRLINES];		/* Array keeping track of baggage count for each passenger */
} LiaisonOfficer_t;

typedef struct{
	int IsBusy;		/* bool controlling whether the screening officer is busy or not */
	int ScreenPass;		/* If the current Passenger Passed Screening, Given to Security Officer */
	int number;
} ScreeningOfficer_t;

typedef struct {
	int PassedPassengers;		/* Number of passengers that passed security */
	int didPassScreening;		/* If the current passenger passed screening */
	int SecurityPass;			/* If the current passenger passed security */
	int TotalPass;				/* If the current passenger passed both screening and security */
	int number;
}SecurityOfficer_t;

typedef struct{
	int name;
	int onBreak;
	int weight[MAX_AIRLINES];
	int count[MAX_AIRLINES];
} CargoHandler_t;

typedef struct{
	int CargoHandlerTotalWeight[MAX_AIRLINES];
	int CargoHandlerTotalCount[MAX_AIRLINES];
	int CIOTotalCount[MAX_AIRLINES];
	int CIOTotalWeight[MAX_AIRLINES];
	int LiaisonTotalCount[MAX_AIRLINES];
	int liaisonPassengerCount;
	int checkInPassengerCount;
	int securityPassengerCount;
} AirportManager_t;

typedef struct{		/* Information passed between Liaison Officer and Passengers */
	int baggageCount;
	int airline;
	int passengerName;
} LiaisonPassengerInfo_t;

typedef struct {		/* Information passed between CheckIn Officer and Passenger */
	int baggageCount;
	int passenger;
	int IsEconomy;
	int seat;
	int gate;
	int line;
	Baggage_t bag[2];
}CheckInPassengerInfo_t;

typedef struct{		/* Information passed between Screening Officer and Passenger */
	int passenger;
	int SecurityOfficer;
} ScreenPassengerInfo_t;

typedef struct{		/* Information passed between Security Officer and Screening Officer */
	int ScreenLine;
} SecurityScreenInfo_t;

typedef struct {		/* Information passed between Security and Passenger */
	int PassedSecurity;
	int questioning;
	int passenger;
}SecurityPassengerInfo_t;

int gates[3];		/* Tracks gate numbers for each airline*/
int ScreeningResult[PASSENGER_COUNT];

LiaisonOfficer_t liaisonOfficers[MAX_LIAISONS];		/* Array of Liaison Officers*/
CheckInOfficer_t CheckIn[MAX_CIOS*MAX_AIRLINES];		/* Array of CheckIn Officers*/
SecurityOfficer_t Security[MAX_SCREEN];		/* Array of Security Officers*/
ScreeningOfficer_t Screen[MAX_SCREEN];		/* Array of Screening Officers*/
CargoHandler_t cargoHandlers[MAX_CARGOHANDLERS];				/* Array of Cargo Handlers*/
LiaisonPassengerInfo_t LPInfo[MAX_LIAISONS];		/* Array of Structs that contain info from passenger to Liaison*/
CheckInPassengerInfo_t CPInfo[MAX_CIOS*MAX_AIRLINES];		/* Array of Structs that contain info from pasenger to CheckIn*/
ScreenPassengerInfo_t SPInfo[MAX_SCREEN+1];		/* Array of Structs that contain info from screening to passenger*/
SecurityScreenInfo_t SSInfo[MAX_SCREEN];		/* Array of structs that contains info from security to screener */
SecurityPassengerInfo_t SecPInfo[MAX_SCREEN];		/* Array of structs that contains info from security to passenger */

int baggageShield[100];
Baggage_t conveyor[PASSENGER_COUNT*2];		/* Conveyor queue that takes bags from the CheckIn and is removed by Cargo Handlers*/
int baggageShiled2[100];
int aircraftBaggageCount[MAX_AIRLINES];		/* Number of baggage on a single airline */
int aircraftBaggageWeight[MAX_AIRLINES];		/* Weight of baggage on a single airline */

int boardingLounges[MAX_AIRLINES];		/* Array of count of people waiting in airport lounge for airline to leave */
int totalPassengersOfAirline[MAX_AIRLINES];		/* Total passengers that should be on an airline */
int totalBaggage[MAX_AIRLINES];
int totalWeight[MAX_AIRLINES];
int ticketsIssued[MAX_AIRLINES];
int liaisonBaggageCount[MAX_AIRLINES];			/* baggage count from liaison's perspective, per each airline */
int alreadyBoarded[MAX_AIRLINES];
int execLineNeedsHelp[MAX_AIRLINES];

int simNumOfPassengers;
int simNumOfAirlines ;
int simNumOfLiaisons;
int simNumOfCIOs;
int simNumOfCargoHandlers;
int simNumOfScreeningOfficers;
int simSeatsPerPlane = 4;
int planeCount = 0;
Passenger_t simPassengers[PASSENGER_COUNT];
LiaisonOfficer_t simLiaisons[MAX_LIAISONS];
CheckInOfficer_t simCIOs[MAX_CIOS];
CargoHandler_t simCargoHandlers[MAX_CARGOHANDLERS];
ScreeningOfficer_t simScreeningOfficers[MAX_SCREEN];
SecurityOfficer_t simSecurityOfficers[MAX_SCREEN];
AirportManager_t simAirportManager;

/*----------------------------------------------------------------------
// Set up
//---------------------------------------------------------------------- */

void setupAirlines(int airlineCount) {
	int i;
	for(i = 0; i < MAX_CIOS*MAX_AIRLINES+MAX_AIRLINES; i++){
		CheckInLine[i] = 0;
	}
	for(i =0; i < MAX_LIAISONS; i++){
		liaisonLine[i] = 0;
	}
	for(i = 0; i < MAX_SCREEN; i++){
		SecurityAvailability[i] = true;
		SecurityLine[i] = 0;
		ScreenLine[i] = 0;
	}

	for (i = 0; i < airlineCount; i++){
		gates[i] = i;
		boardingLounges[i] = 0;
		totalPassengersOfAirline[i] = simNumOfPassengers/simNumOfAirlines;
		aircraftBaggageCount[i] = 0;		/* Number of baggage on a single airline */
		aircraftBaggageWeight[i] = 0;		/* Weight of baggage on a single airline */
		execLineNeedsHelp[i] = true;
	}
	
	if(simNumOfPassengers%simNumOfAirlines > 0){
		totalPassengersOfAirline[0] += simNumOfPassengers%simNumOfAirlines;
	}
	
	for (i = 0; i < airlineCount; i++){
		/* LiaisonSeat[i] = AIRLINE_SEAT; */
		gateLocks[i] = CreateLock("Gate Lock");
		gateLocksCV[i] = CreateCV("Gate CV");
		execLineLocks[i] = CreateLock("Exec Line Lock");
		execLineCV[i] = CreateCV("Exec Line CV");
	}
	
	for (i = 0; i < airlineCount * AIRLINE_SEAT; i++){
		ScreeningResult[i] = true;
	}
}

void createPassengers(int quantity) {
	int i;

	for(i = 0; i < quantity; i++) {
		simPassengers[i].NotTerrorist = true;
		simPassengers[i].gate = -1;
		simPassengers[i].name = i;
		simPassengers[i].seat = -1;
		simPassengers[i].airline = -1;
		simPassengers[i].myLine = -1;
		simPassengers[i].baggageCount = 2;
		simPassengers[i].bags[1].weight = rand() % 31 + 30;
		simPassengers[i].bags[0].weight = rand() % 31 + 30;
		simPassengers[i].bags[1].airline = -1;
		simPassengers[i].bags[0].airline = -1;
		
		simPassengers[i].economy = true;				/* Default is economy class */
		if(rand() % 3 == 1){		/* 25% of being executive class */
			simPassengers[i].economy = false;
		}
	}
}

void createLiaisons(int quantity) {
	int i;

	for(i = 0; i < quantity; i++) {
		liaisonLine[i] = 0;
		liaisonLineCV[i] = CreateCV("Liaison Line CV " + i);
		liaisonOfficerCV[i] = CreateCV("Liaison Officer CV " + 1);
		liaisonLineLocks[i] = CreateLock("Liaison Line Lock " + i);
		liaisonOfficers[i].airline = -1;
		liaisonOfficers[i].number = i;
		liaisonOfficers[i].passengerCount = 0;
		liaisonOfficers[i].airlineBaggageCount[0] = 0;
		liaisonOfficers[i].airlineBaggageCount[1] = 0;
		liaisonOfficers[i].airlineBaggageCount[2] = 0;
	}
}

void setupEconomyCIOs(int airlineCount, int quantity) {
	int i;
	for (i = 0; i < simNumOfAirlines*simNumOfCIOs; i++){
		CheckInLine[i] = 0;
		CheckInLocks[i] = CreateLock("CheckIn Officer Lock");
		CheckInBreakCV[i] = CreateCV("CheckIn Break Time CV");
		CheckInCV[i] = CreateCV("CheckIn Line CV");
		CheckInOfficerCV[i] = CreateCV("CheckIn Officer CV");
	}
}

void setupExecutiveCIOs(int airlineCount, int quantity) {
	int i;
	for(i = simNumOfAirlines*simNumOfCIOs; i < simNumOfAirlines*simNumOfCIOs + simNumOfAirlines; i++) {
		CheckInLine[i] = 0;
		CheckInLocks[i] = CreateLock("CheckIn Officer Lock");
		CheckInCV[i] = CreateCV("CheckIn Line CV");
	}
}

void createCIOs(int airlineCount, int quantity) {
	int i;
	for(i = 0; i < simNumOfAirlines*simNumOfCIOs; i++) {
		CheckIn[i].airline = i/simNumOfCIOs;
		CheckIn[i].passengerCount = 0;		/* Passenger Count */
		CheckIn[i].OnBreak = false;		/* Controls break time */
		CheckIn[i].work = true;
		CheckIn[i].number = i;
	}
}

void createAirportManager() {
	AirportManager_t simAirportManager;
}

void createSecurityAndScreen(int quantity) {
	int i;
	for (i = 0; i < quantity; i++){
		SecurityLine[i] = 0;
		Security[i].PassedPassengers = 0;
		Security[i].didPassScreening = false;
		Security[i].SecurityPass = false;
		Security[i].TotalPass = false;
		Security[i].number = i;
		Screen[i].IsBusy = false;
		Screen[i].number = i;
		Screen[i].ScreenPass = false;
		ScreenOfficerCV[i] = CreateCV("Screen Officer CV");
		SecurityOfficerCV[i] = CreateCV("Security Officer CV");
		ScreenLocks[i] = CreateLock("Screen Lock");
		SecurityLocks[i] = CreateLock("Security Lock");
		SecurityLineCV[i] = CreateCV("Security line CV");
		simScreeningOfficers[i] = Screen[i];
		simSecurityOfficers[i] = Security[i];
	}
	
	for (i = 0; i < quantity;i++){
		SecurityAvailability[i] = true;
	}
	
	ScreenLineCV[0] = CreateCV("Screen Line CV");
}

void createCargoHandlers(int quantity) {
	int i;
	for(i = 0; i < quantity; i++) {
		cargoHandlers[i].weight[0] = 0;
		cargoHandlers[i].weight[1] = 0;
		cargoHandlers[i].weight[2] = 0;
		cargoHandlers[i].count[0] = 0;
		cargoHandlers[i].count[1] = 0;
		cargoHandlers[i].count[1] = 0;
		cargoHandlers[i].name = i;
		cargoHandlers[i].onBreak = false;
	}
}

void setupBaggageAndCargo(int airlineCount) {
	int i;
	for (i = 0; i < airlineCount; i++){
		AirlineBaggage[i] = CreateLock("Airline Baggage Lock");
	}
	CargoHandlerLock = CreateLock("Cargo Handler Lock");
	CargoHandlerCV = CreateCV("Cargo Handler CV ");
	for(i = 0; i < PASSENGER_COUNT*2; i++){
		conveyor[i].weight = 0;
		conveyor[i].airline = -1;  
	}
}

void setupSingularLocks() {
	Passenger_ID_Lock = CreateLock("");
	Liaison_ID_Lock = CreateLock("");
	CheckIn_ID_Lock = CreateLock("");
	Screening_ID_Lock = CreateLock("");
	Security_ID_Lock = CreateLock("");
	Cargo_ID_Lock = CreateLock("");
	liaisonLineLock = CreateLock("Liaison Line Lock");
	CheckInLock = CreateLock("CheckIn Line Lock");
	ScreenLines = CreateLock("Screen Line Lock");
	airlineSeatLock = CreateLock("Airline Seat Lock");
	/* LiaisonSeats = CreateLock("Liaison Seat Lock"); */
	seatLock = CreateLock("Seat Lock");
	BaggageLock = CreateLock("Baggage Lock");
	SecurityAvail = CreateLock("Security Availability lock");
	SecurityLines = CreateLock("Security Line Lock");
}

void Initialize(){		
	simNumOfPassengers = PASSENGER_COUNT;
	simNumOfCargoHandlers = MAX_CARGOHANDLERS;
	simNumOfAirlines = MAX_AIRLINES;
	simNumOfCIOs = MAX_CIOS;
	simNumOfLiaisons = MAX_LIAISONS;
	simNumOfScreeningOfficers = MAX_SCREEN;
	
	for(i = 0; i < simNumOfAirlines; i++) {
		liaisonBaggageCount[i] = 0;
		ticketsIssued[i] = 0;
		alreadyBoarded[i] = false;
	}
	
	createAirportManager();
	setupAirlines(simNumOfAirlines);
	setupSingularLocks();
	setupEconomyCIOs(simNumOfAirlines, simNumOfCIOs);
	setupExecutiveCIOs(simNumOfAirlines, simNumOfCIOs);
	createCIOs(simNumOfAirlines, simNumOfCIOs);
	createLiaisons(simNumOfLiaisons);
	createSecurityAndScreen(simNumOfScreeningOfficers);
	setupBaggageAndCargo(simNumOfAirlines);
	createCargoHandlers(simNumOfCargoHandlers);
	createPassengers(simNumOfPassengers);
	printf((int)"Created everything\n", sizeof("Created everything\n"), 0, 0);
}






