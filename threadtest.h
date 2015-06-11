#include "copyright.h"
#include "system.h"
#include "synch.h"
#include <list>
#include <vector>
#include <deque>


#define LIAISONLINE_COUNT 5 // Number of Liaison Officers
#define CHECKIN_COUNT 5  // Number of CheckIn Officers
#define AIRLINE_COUNT 3  // Number of Airlines
#define SCREEN_COUNT 4		// Number of Screening and Security Officers

void SimpleThread(int which);
void ThreadTest();

//----------------------------------------------------------------------
// Arrays, Lists, and Vectors
//----------------------------------------------------------------------
int liaisonLine[LIAISONLINE_COUNT];		// Array of line sizes for each Liaison Officer
int CheckInLine[CHECKIN_COUNT * AIRLINE_COUNT];		// Array of line sizes for each CheckIn Officer
int ScreenLine[1];		// Array of line sizes for each Screening Officer
Condition *ScreenLineCV[1];			// Condition Variables for the Screening Line
Condition *ScreenOfficerCV[SCREEN_COUNT];		// Condition Variables for each Screening Officer
Condition *SecurityOfficerCV[SCREEN_COUNT];		// Condition Variables for each Security Officer
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
Lock *CheckInLock;		// Lock to get into CheckIn Line
Lock *ScreenLines;		// Lock to get into Screening Line
Lock *CargoHandlerLock;		// Lock for Cargo Handlers for placing baggage onto the airline
Condition *CargoHandlerCV;		// Condition Variable for Cargo Handlers
bool seats[50*AIRLINE_COUNT] = {true}; // Contains seat numbers for all planes
Lock *airlineSeatLock;		// Lock for find seat number for customers
Lock *BaggageLock;		// Lock for placing Baggage onto the conveyor
Lock *SecurityAvail;		// Lock for seeing if a Security Officer is busy

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
};

struct CheckInPassengerInfo{		// Information passed between CheckIn Officer and Passenger
	int baggageCount;
	int seat;
	int line;
	std::vector<Baggage> bag;		// Vector of bags whereas, customer will append bags and CheckIn Officer will remove
};

struct ScreenPassengerInfo{
	int line;
};

struct SecurityScreenInfo{
	bool PassedScreening;
	int ScreenLine;
};

struct SecurityPassengerInfo{
	bool PassedSecurity;
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
	  
  private:
	  bool NotTerrorist;
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
  
  private:
	struct Liaison{		// Struct containing all important info
	  char* name;
	  int airline;		// Airline the liaison will assign to the passenger
	  int number;		// Number of the liaison (which line they control)
	  int passengerCount;		// Number of passengers the liaison has helped
  std::vector<int>baggageCount;		// Vector keeping track of baggage count for each passenger
	} info; 
};

//----------------------------------------------------------------------
// CheckIn Officer
//----------------------------------------------------------------------
class CheckInOfficer{
	public:
	  CheckInOfficer(int i);
	  ~CheckInOfficer();
	  void setBreak();		// Go on Break
	  bool getBreak();		// For managers to see who is on break
	  void DoWork();
	  int getAirline();		// Returns airline CheckIn Officer is working for
	  int getNumber();		// Returns number of officer (which line they control)
	
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
		int getWeight(){return weight;}
		int getCount(){return count;}
		
	private:
		int name;
		bool onBreak;
		int weight;
		int count;
};

//----------------------------------------------------------------------
// Airport Manager
//----------------------------------------------------------------------
class AirportManager{
	public:
		AirportManager();
		~AirportManager();
		void DoWork();
		void AddCargoHandler(CargoHandler *ch);
		void EndOfDay();
		
	private:
		std::vector<CargoHandler*> cargoHandlers;
		int CargoHandlerTotalWeight;
		int CargoHandlerTotalCount;
		
};
//----------------------------------------------------------------------
// Screening Officer
//----------------------------------------------------------------------
class ScreeningOfficer{
	public:
		ScreeningOfficer(int i);
		~ScreeningOfficer();
		void DoWork();
	
	private:
		char* name;
		bool ScreenPass;
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
		bool getAvail(){return available;}
		
	private:
		bool available;
		char* name;
		int PassedPassengers;
		bool didPassScreening;
		bool SecurityPass;
		bool TotalPass;
		int number;
};

