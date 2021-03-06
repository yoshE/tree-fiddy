#include "copyright.h"
#include "system.h"
#include "synch.h"
#include <list>
#include <vector>
#include <deque>

#define BAGGAGE_COUNT 2		// Passenger starts with 2 baggages and will randomly have one more
#define BAGGAGE_WEIGHT 30		// Baggage weight starts at 30 and can have 0-30 more lbs added randomly
#define AIRLINE_COUNT 5	// Number of airlines
#define CHECKIN_COUNT 5		// Number of CheckIn Officers
#define PASSENGER_COUNT 150	// Total number of passengers
#define AIRLINE_SEAT 50		// Number of seats per Airline
#define LIAISONLINE_COUNT 7 // Number of Liaison Officers
#define SCREEN_COUNT 5		// Number of Screening and Security Officers
// Max agent consts
#define MAX_PASSENGERS			1000
#define MAX_AIRLINES			5
#define MAX_LIAISONS			7
#define MAX_CIOS				5
#define MAX_CARGOHANDLERS		10
#define MAX_SCREEN				5

void SimpleThread(int which);
void ThreadTest();
void AirportTests();
void RunSim();

//----------------------------------------------------------------------
// Arrays, Lists, and Vectors
//----------------------------------------------------------------------
bool SecurityAvailability[SCREEN_COUNT];		// Array of Bools for availability of each security officer
int liaisonLine[LIAISONLINE_COUNT];		// Array of line sizes for each Liaison Officer
int CheckInLine[CHECKIN_COUNT*AIRLINE_COUNT+AIRLINE_COUNT];		// Array of line sizes for each CheckIn Officer
int SecurityLine[SCREEN_COUNT];		// Array of line sizes for return passengers from security questioning
int ScreenLine[SCREEN_COUNT];		// Array of line sizes for each Screening Officer
Condition *ScreenLineCV[SCREEN_COUNT];			// Condition Variables for the Screening Line
Condition *ScreenOfficerCV[SCREEN_COUNT];		// Condition Variables for each Screening Officer
Condition *SecurityOfficerCV[SCREEN_COUNT];		// Condition Variables for each Security Officer
Condition *SecurityLineCV[SCREEN_COUNT];		// Condition Variables for returning passengers from questioning
Condition *liaisonLineCV[LIAISONLINE_COUNT];		// Condition Variables for each Liaison Line
Condition *liaisonOfficerCV[LIAISONLINE_COUNT];		// Condition Variables for each Liaison Officer
Condition *CheckInCV[CHECKIN_COUNT*AIRLINE_COUNT+AIRLINE_COUNT];		// Condition Variables for each CheckIn Line
Condition *CheckInOfficerCV[CHECKIN_COUNT*AIRLINE_COUNT];		// Condition Variables for each CheckIn Officer
Condition *CheckInBreakCV[CHECKIN_COUNT*AIRLINE_COUNT];		// Condition Variables for each CheckIn Officer Break Time
Lock *liaisonLineLock;		// Lock to get into a liaison Line
Lock *liaisonLineLocks[LIAISONLINE_COUNT];		// Array of Locks for Liaison Officers
Lock *CheckInLocks[CHECKIN_COUNT*AIRLINE_COUNT];		//Array of Locks for CheckIn Officers
Lock *ScreenLocks[SCREEN_COUNT];		// Array of Locks for Screening Officers
Lock *SecurityLocks[SCREEN_COUNT];		// Array of Locks for Security Officers
Lock *AirlineBaggage[AIRLINE_COUNT]; 		// Array of Locks for placing baggage on airlines
Lock *CheckInLock;		// Lock to get into CheckIn Line
Lock *ScreenLines;		// Lock to get into Screening Line
Lock *CargoHandlerLock;		// Lock for Cargo Handlers for taking baggage off conveyor
Condition *CargoHandlerCV;		// Condition Variable for Cargo Handlers
Lock *airlineSeatLock;		// Lock for find seat number for customers
Lock *BaggageLock;		// Lock for placing Baggage onto the conveyor
Lock *SecurityAvail;		// Lock for seeing if a Security Officer is busy
Lock *SecurityLines;			// Lock for returning passengers from Security
Lock *gateLocks[AIRLINE_COUNT];				//Locks for waiting at the gate
Condition *gateLocksCV[AIRLINE_COUNT];		//CVs for waiting at the gate
Lock *seatLock;			// Lock for assigned airline seats in the Liaison
Lock *execLineLocks[MAX_AIRLINES];
Condition *execLineCV[MAX_AIRLINES];
//----------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------
struct Baggage{		// Contains information for 1 piece of baggage
	int airlineCode;		// Airline the baggage should be placed on
	int weight;
};

struct LiaisonPassengerInfo{		// Information passed between Liaison Officer and Passengers
	int baggageCount;
	int airline;
	int passengerName;
};

struct CheckInPassengerInfo{		// Information passed between CheckIn Officer and Passenger
	int baggageCount;
	int passenger;
	bool IsEconomy;
	int seat;
	int gate;
	int line;
	std::vector<Baggage> bag;		// Vector of bags whereas, customer will append bags and CheckIn Officer will remove
};

struct ScreenPassengerInfo{		// Information passed between Screening Officer and Passenger
	int passenger;
	int SecurityOfficer;
};

struct SecurityScreenInfo{		// Information passed between Security Officer and Screening Officer
	int ScreenLine;
};

struct SecurityPassengerInfo{		// Information passed between Security and Passenger
	bool PassedSecurity;
	bool questioning;
	int passenger;
};

//----------------------------------------------------------------------
// Passenger
//----------------------------------------------------------------------
class Passenger {
	public:
		Passenger(int n);
		~Passenger();
		char getName() { return name;}			// debugging assist
		int getAirline() {return airline;}		// Gets Airline
		bool getClass() {return economy;}		// Gets Economy or Executive Class
		void ChooseLiaisonLine();
		int getBaggageCount() {return baggageCount;}		// Returns number of baggage
		std::vector<Baggage> getBags() {return bags;}		// Vector of bag structs
	  
  private:
	  bool NotTerrorist;
	  int gate;
	  int name;        // useful for debugging
	  int seat;			// Seat Number
	  int airline;		//which airline does the passenger fly
	  bool economy;		//economy or executive ticket
	  int myLine;		//which line the passenger got into
	  int baggageCount;		// Number of baggage
	  std::vector<Baggage> bags;	// Vector containing baggage items
};

//----------------------------------------------------------------------
// Liaison Officer
//----------------------------------------------------------------------
class LiaisonOfficer {
  public:
    LiaisonOfficer(int i);
	~LiaisonOfficer();
	void DoWork();
	int getPassengerCount(); // For manager to get passenger headcount
	int getPassengerBaggageCount(int n); // For manager to get passenger bag count
	int getAirlineBaggageCount(int n);		// Returns baggage count per airline
  
  private:
	struct Liaison{		// Struct containing all important info
	  char* name;
	  int airline;		// Airline the liaison will assign to the passenger
	  int number;		// Number of the liaison (which line they control)
	  int passengerCount;		// Number of passengers the liaison has helped
	  int airlineBaggageCount[AIRLINE_COUNT];		// Array keeping track of baggage count for each passenger
	} info; 
};

//----------------------------------------------------------------------
// CheckIn Officer
//----------------------------------------------------------------------
class CheckInOfficer{
	public:
	  CheckInOfficer(int i);
	  ~CheckInOfficer();
	  void setOffBreak();
	  bool getOnBreak();		// For managers to see who is on break
	  void DoWork();
	  int getAirline();		// Returns airline CheckIn Officer is working for
	  int getNumber();		// Returns number of officer (which line they control)
	  std::vector<Baggage> totalBags;
	  bool OnBreak;
	  int getPassengerCount(){return info.passengerCount;}
	
	private:
	  struct CheckIn{
		char* name;
		int number;
		int passengerCount;
		std::vector<Baggage> bags;		// Vector of bags that is appended by Passenger and removed by this Officer
		int airline;
		bool OnBreak;		// If there are no passengers in line, the officer goes on break until manager makes them up
		bool work;		// Always working until all passengers have finished checking in
	  }info;
};

//----------------------------------------------------------------------
// Cargo Handler
//----------------------------------------------------------------------
class CargoHandler{
	public:
		CargoHandler(int n);
		~CargoHandler();
		char getName(){return name;}
		bool getBreak(){return onBreak;}
		void DoWork();
		int getWeight(int n){return weight[n];}
		int getCount(int n){return count[n];}
		
	private:
		int name;
		bool onBreak;
		int weight[AIRLINE_COUNT];
		int count[AIRLINE_COUNT];
};

//----------------------------------------------------------------------
// Airport Manager
//----------------------------------------------------------------------
class AirportManager{
	public:
		AirportManager();
		~AirportManager();
		void DoWork();
		void EndOfDay();
		
	private:
		int CargoHandlerTotalWeight[AIRLINE_COUNT];
		int CargoHandlerTotalCount[AIRLINE_COUNT];
		int CIOTotalCount[AIRLINE_COUNT];
		int CIOTotalWeight[AIRLINE_COUNT];
		int LiaisonTotalCount[AIRLINE_COUNT];
		int liaisonPassengerCount;
		int checkInPassengerCount;
		int securityPassengerCount;
};
//----------------------------------------------------------------------
// Screening Officer
//----------------------------------------------------------------------
class ScreeningOfficer{
	public:
		ScreeningOfficer(int i);
		~ScreeningOfficer();
		void DoWork();
		bool getBusy() {return IsBusy;}		// Sets available
		void setBusy() {IsBusy = true;}		// Returns available
	
	private:
		char* name;
		bool IsBusy;		// bool controlling whether the screening officer is busy or not
		bool ScreenPass;		// If the current Passenger Passed Screening, Given to Security Officer
		int number;
};

//----------------------------------------------------------------------
// Security Officer
//----------------------------------------------------------------------
class SecurityOfficer{
	public:
		SecurityOfficer(int i);
		~SecurityOfficer();
		void DoWork();
		int getPassengers(){return PassedPassengers;}
		
	private:
		int PassedPassengers;		// Number of passengers that passed security
		bool didPassScreening;		// If the current passenger passed screening
		bool SecurityPass;			// If the current passenger passed security
		bool TotalPass;				// If the current passenger passed both screening and security
		int number;
};
