#include "syscall.h"


#define true 1
#define false 0

/* Max agent consts */
#define PASSENGER_COUNT			6
#define MAX_AIRLINES			3
#define MAX_LIAISONS			2
#define MAX_CIOS				1
#define MAX_CARGOHANDLERS		2
#define MAX_SCREEN				2
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
	int i, x;

	for (i = 0; i < airlineCount; i++){
		gates[i] = CreateMV("Gates ", i, i);
		boardingLounges[i] = CreateMV("BoardingLounges ", 0, i);
		totalPassengersOfAirline[i] = CreateMV("totalPAssengersOfAirline ", simNumOfPassengers/simNumOfAirlines, i);
		aircraftBaggageCount[i] = CreateMV("aircraftBaggageCount ", 0, i);		/* Number of baggage on a single airline */
		aircraftBaggageWeight[i] = CreateMV("aircraftBaggageWeight ", 0, i);		/* Weight of baggage on a single airline */
		execLineNeedsHelp[i] = CreateMV("execLineNeedsHelp ", 1, i);
	}
	
	if(simNumOfPassengers%simNumOfAirlines > 0){
		x = GetMV(totalPassengersOfAirline[i]);
		x += simNumOfPassengers%simNumOfAirlines;
		SetMV(totalPassengersOfAirline[i], x);
	}
	
	for (i = 0; i < airlineCount; i++){
		gateLocks[i] = CreateLock("GateLock ", i);
		gateLocksCV[i] = CreateCV("GateCV ", i);
		execLineLocks[i] = CreateLock("ExecLineLock ", i);
		execLineCV[i] = CreateCV("ExecLineCV", i);
	}
	
	for (i = 0; i < airlineCount * AIRLINE_SEAT; i++){
		ScreeningResult[i] = CreateMV("ScreeningResult ", 1, i);
	}
}

void createPassengers(int quantity) {
	int i;

	for(i = 0; i < quantity; i++) {
		simPassengers[i].NotTerrorist = CreateMV("simPassengersTerrorist ", 1, i);
		simPassengers[i].gate = CreateMV("simPassengersGate ", -1, i);
		simPassengers[i].name = CreateMV("simPassengersName ", i, i);
		simPassengers[i].seat = CreateMV("simPassengersSeat ", -1, i);
		simPassengers[i].airline = CreateMV("simPassengersAirline", -1, i);
		simPassengers[i].myLine = CreateMV("simPassengersMyLine ", -1, i);
		simPassengers[i].baggageCount = CreateMV("simPassengersBaggageCount ", 2, i);
		simPassengers[i].bags[1].weight = CreateMV("simPassengersBag2Weight ", rand() % 31 + 30, i);
		simPassengers[i].bags[0].weight = CreateMV("simPassengersBag1Weight ", rand() % 31 + 30, i);
		simPassengers[i].bags[1].airline = CreateMV("simPassengersBag2Airline ", -1, i);
		simPassengers[i].bags[0].airline = CreateMV("simPassengersBag1Airline ", -1, i);
		
		simPassengers[i].economy = CreateMV("simPassengersEconomy ", 1, i);				/* Default is economy class */
		if(rand() % 3 == 1){		/* 25% of being executive class */
			SetMV(simPassengers[i].economy, 0);
		}
	}
}

void createLiaisons(int quantity) {
	int i;

	for(i = 0; i < quantity; i++) {
		liaisonLine[i] = CreateMV("LiaisonLine ", 0, i);
		liaisonLineCV[i] = CreateCV("LiaisonLineCV ", i);
		liaisonOfficerCV[i] = CreateCV("LiaisonOfficerCV ", i);
		liaisonLineLocks[i] = CreateLock("LiaisonLineLock ", i);
		liaisonOfficers[i].airline = CreateMV("LiaisonOfficersAirline ", -1, i);
		liaisonOfficers[i].number = CreateMV("LiaisonOfficersNumber ", i, i);
		liaisonOfficers[i].passengerCount = CreateMV("LiaisonOfficerPassengerCount ", 0, i);
		liaisonOfficers[i].airlineBaggageCount[0] = CreateMV("LiaisonOfficerBaggageCount1 ", 0, i);
		liaisonOfficers[i].airlineBaggageCount[1] = CreateMV("LiaisonOfficerBaggageCount2 ", 0, i);
		liaisonOfficers[i].airlineBaggageCount[2] = CreateMV("LiaisonOfficerBaggageCount3 ", 0, i);

		LPInfo[i].baggageCount = CreateMV("LPInfoBaggageCount ", 0, i);		/* Array of Structs that contain info from passenger to Liaison*/
		LPInfo[i].airline = CreateMV("LPInfoAirline ", 0, i);
		LPInfo[i].passengerName = CreateMV("LPInfoPassengerName ", 0, i);
	}
}

void setupCIOs(int airlineCount, int quantity) {
	int i;
	for (i = 0; i < simNumOfAirlines*simNumOfCIOs; i++){
		CheckInLine[i] = CreateMV("CheckInLine ", 0, i);
		CheckInLocks[i] = CreateLock("CheckInOfficerLock ", i);
		CheckInBreakCV[i] = CreateCV("CheckInBreakTimeCV ", i);
		CheckInCV[i] = CreateCV("CheckInLineCV ", i);
		CheckInOfficerCV[i] = CreateCV("CheckInOfficerCV ", i);

		CPInfo[i].baggageCount = CreateMV("CPInfoBaggageCount ", 0, i);
		CPInfo[i].passenger = CreateMV("CPInfoPassenger ", 0, i);
		CPInfo[i].IsEconomy = CreateMV("CPInfoIsEconomy ", 1, i);
		CPInfo[i].seat = CreateMV("CPInfoSeat ", 0, i);
		CPInfo[i].gate = CreateMV("CPInfoGate ", 0, i);
		CPInfo[i].line = CreateMV("CPInfoLine ", 0, i);
		CPInfo[i].bag[0].weight = CreateMV("CPInfoBag1Weight ", 0, i);
		CPInfo[i].bag[0].airline = CreateMV("CPInfoBag1Airline ", 0, i);
		CPInfo[i].bag[1].weight = CreateMV("CPInfoBag2Weight ", 0, i);
		CPInfo[i].bag[2].weight = CreateMV("CPInfoBag2Airline ", 0, i);
		
		CheckIn[i].airline = CreateMV("CheckInAirline ", i/simNumOfCIOs, i);
		CheckIn[i].passengerCount = CreateMV("CheckInPassengerCount ", 0, i);		/* Passenger Count */
		CheckIn[i].OnBreak = CreateMV("CheckInOnBreak ", 0, i);		/* Controls break time */
		CheckIn[i].work = CreateMV("CheckInWork ", 1, i);
		CheckIn[i].number = CreateMV("CheckInNumber ", i, i);
	}
	
	for(i = simNumOfAirlines*simNumOfCIOs; i < simNumOfAirlines*simNumOfCIOs + simNumOfAirlines; i++) {
		CheckInLine[i] = CreateMV("CheckInLine ", 0, i);
		CheckInLocks[i] = CreateLock("CheckInOfficerLock ", i);
		CheckInCV[i] = CreateCV("CheckInLineCV ", i);
	}
}

void createAirportManager() {
	AirportManager_t simAirportManager;
}

void createSecurityAndScreen(int quantity) {
	int i;
	for (i = 0; i < quantity; i++){	
		SecurityAvailability[i] = CreateMV("SecurityAvailability ", 1, i);
		SecurityLine[i] = CreateMV("SecurityLine ", 0, i);
		ScreenLine[i] = CreateMV("ScreenLine ", 0, i);
		
		Security[i].PassedPassengers = CreateMV("SecurityPassedPassengers ", 0, i);
		Security[i].didPassScreening = CreateMV("SecurityDidPassScreening ", 0, i);
		Security[i].SecurityPass = CreateMV("SecuritySecurityPass ", 0, i);
		Security[i].TotalPass = CreateMV("SecurityTotalPass ", 0, i);
		Security[i].number = CreateMV("SecurityNumber ", i, i);
		
		Screen[i].IsBusy = CreateMV("ScreenIsBusy ", 0, i);
		Screen[i].number = CreateMV("ScreenNumber ", i, i);
		Screen[i].ScreenPass = CreateMV("ScreenScreenPass ", 0, i);
		
		ScreenOfficerCV[i] = CreateCV("ScreenOfficerCV ", i);
		SecurityOfficerCV[i] = CreateCV("SecurityOfficerCV ", i);
		ScreenLocks[i] = CreateLock("ScreenLock ", i);
		SecurityLocks[i] = CreateLock("SecurityLock ", i);
		SecurityLineCV[i] = CreateCV("SecuritylineCV ", i);
		simScreeningOfficers[i] = Screen[i];
		simSecurityOfficers[i] = Security[i];
		
		SSInfo[i].ScreenLine = CreateMV("SSInfoScreenLine ", 0, i);

		SecPInfo[i].PassedSecurity = CreateMV("SecPInfoPassedSecurity ", 0, i);
		SecPInfo[i].questioning = CreateMV("SecPInfoQuestioning ", 0, i);
		SecPInfo[i].passenger = CreateMV("SecPInfoPassenger ", 0, i);
	}
	
	for (i = 0; i < quantity + 1; i ++){
		SPInfo[i].passenger = CreateMV("SPInfoPassenger ", 0, i);
		SPInfo[i].SecurityOfficer = CreateMV("SPInfoSecurityOfficer ", 0, i);
	}
	
	ScreenLineCV[0] = CreateCV("ScreenLineCV ", 0);
}

void createCargoHandlers(int quantity) {
	int i;
	for(i = 0; i < quantity; i++) {
		cargoHandlers[i].weight[0] = CreateMV("CargoHandler1Weight ", 0, i);
		cargoHandlers[i].weight[1] = CreateMV("CargoHandler2Weight ", 0, i);
		cargoHandlers[i].weight[2] = CreateMV("CargoHandler3Weight ", 0, i);
		cargoHandlers[i].count[0] = CreateMV("CargoHandler1Count ", 0, i);
		cargoHandlers[i].count[1] = CreateMV("CargoHandler2Count ", 0, i);
		cargoHandlers[i].count[2] = CreateMV("CargoHandler3Count ", 0, i);
		cargoHandlers[i].name = CreateMV("CargoHandlerName ", i, i);
		cargoHandlers[i].onBreak = CreateMV("CargoHandlerOnBreak ", 0, i);
	}
}

void setupBaggageAndCargo(int airlineCount) {
	int i;
	for (i = 0; i < airlineCount; i++){
		AirlineBaggage[i] = CreateLock("AirlineBaggageLock ", i);
	}
	CargoHandlerLock = CreateLock("CargoHandlerLock ", 1);
	CargoHandlerCV = CreateCV("CargoHandlerCV ", 1);
	for(i = 0; i < PASSENGER_COUNT*2; i++){
		conveyor[i].weight = CreateMV("ConveyorWeight ", 0, i);
		conveyor[i].airline = CreateMV("ConveyorAirline ", 0, i);  
	}
}

void setupSingularLocks() {
	Passenger_ID_Lock = CreateLock("PassengerIDLock ", 1);
	Liaison_ID_Lock = CreateLock("LiaisonIDLock ", 1);
	CheckIn_ID_Lock = CreateLock("CheckInIDLock ", 1);
	Screening_ID_Lock = CreateLock("ScreeningIDLock ", 1);
	Security_ID_Lock = CreateLock("SecurityIDLock ", 1);
	Cargo_ID_Lock = CreateLock("CargoIDLock ", 1);
	liaisonLineLock = CreateLock("LiaisonLineLock ", 1);
	CheckInLock = CreateLock("CheckInLineLock ", 1);
	ScreenLines = CreateLock("ScreenLineLock ", 1);
	airlineSeatLock = CreateLock("AirlineSeatLock ", 1);
	seatLock = CreateLock("SeatLock ", 1);
	BaggageLock = CreateLock("BaggageLock ", 1);
	SecurityAvail = CreateLock("SecurityAvailabilityLock ", 1);
	SecurityLines = CreateLock("SecurityLineLock ", 1);
}

void Initialize(){		
	int i;
	simNumOfPassengers = PASSENGER_COUNT;
	simNumOfCargoHandlers = MAX_CARGOHANDLERS;
	simNumOfAirlines = MAX_AIRLINES;
	simNumOfCIOs = MAX_CIOS;
	simNumOfLiaisons = MAX_LIAISONS;
	simNumOfScreeningOfficers = MAX_SCREEN;
	
	printf((int)"Start creating everything\n", sizeof("Start creating everything\n"), 0, 0);
	
	for(i = 0; i < simNumOfAirlines; i++) {
		printf((int)"Start creating loop\n", sizeof("Start creating loop\n"), 0, 0);
		liaisonBaggageCount[i] = CreateMV("LiaisonBaggageCount ", 0, i);
		ticketsIssued[i] = CreateMV("TicketsIssued ", 0, i);
		alreadyBoarded[i] = CreateMV("alreadyBoarded ", 0, i);
	}
	
	Passenger_ID = CreateMV("PassengerID ", 0, 0);
	Liaison_ID = CreateMV("LiaisonID ", 0, 0);
	Screening_ID = CreateMV("ScreenID ", 0, 0);
	CheckIn_ID = CreateMV("CheckInID ", 0, 0);
	Security_ID = CreateMV("SecurityID ", 0, 0);
	Cargo_ID = CreateMV("CargoID ", 0, 0);
	
	printf((int)"Start creating Airport stuff\n", sizeof("Start creating Airport stuff\n"), 0, 0);
	
	createAirportManager();
	setupAirlines(simNumOfAirlines);
	setupSingularLocks();
	setupCIOs(simNumOfAirlines, simNumOfCIOs);
	createLiaisons(simNumOfLiaisons);
	createSecurityAndScreen(simNumOfScreeningOfficers);
	setupBaggageAndCargo(simNumOfAirlines);
	createCargoHandlers(simNumOfCargoHandlers);
	createPassengers(simNumOfPassengers);
	printf((int)"Created everything\n", sizeof("Created everything\n"), 0, 0);
}



