#include "copyright.h"
#include "system.h"
#include "synch.h"
#include <list>
#include <vector>
#include <deque>


#define LIAISONLINE_COUNT 5
#define CHECKIN_COUNT 5
#define AIRLINE_COUNT 3

void SimpleThread(int which);

void ThreadTest();
int liaisonLine[LIAISONLINE_COUNT];
Condition *liaisonLineCV[LIAISONLINE_COUNT];
Condition *liaisonOfficerCV[LIAISONLINE_COUNT];
Condition *CheckInCV[CHECKIN_COUNT*AIRLINE_COUNT+AIRLINE_COUNT];
Condition *CheckInOfficerCV[CHECKIN_COUNT*AIRLINE_COUNT];
Condition *CheckInBreakCV[CHECKING_COUNT*AIRLINE_COUNT];
Lock *liaisonLineLock;
Lock *liaisonLineLocks[LIAISONLINE_COUNT];
Lock *CheckInLocks[CHECKIN_COUNT*AIRLINE_COUNT];
Lock *CheckInLock;
Lock *CargoHandlerLock;
Condition *CargoHandlerCV;
bool seats[50*AIRLINE_COUNT] = {true}; // Contains seat numbers for all planes
Lock *airlineSeatLock;
Lock *BaggageLock;

struct Baggage{
	int airlineCode;
	int weight;
};

struct LiaisonPassengerInfo{
	int baggageCount;
	int airline;
}

struct CheckInPassengerInfo{
	int baggageCount;
	int seat;
	std::vector<Baggage> bag;
}

class Passenger {
	public:
		Passenger(int n);
		~Passenger();
		char getName() { return name;}			// debugging assist
		int getAirline() {return airline;}
		bool getClass() {return economy;}
		int getTicket() {return economy;}
		void ChooseLiaisonLine();
		int getBaggageCount() {return baggageCount;}
		void setAirline(int n);
		void setSeat(int n);
		void ChooseCheckIn();
	  
  private:
	  int name;        // useful for debugging
	  int seat;			// Seat Number
	  int airline;		//which airline does the passenger fly
	  bool economy;		//economy or executive ticket
	  int myLine;		//which line the passenger got into
	  int baggageCount;
	  std::vector<Baggage> bags;
};

class LiaisonOfficer {
  public:
    LiaisonOfficer(char* deBugName, int i);
	~LiaisonOfficer();
	char* getName();
	int getPassengerCount(); // For manager to get passenger headcount
	int getPassengerBaggageCount(int n); // For manager to get passenger bag count
  
  private:
	struct Liaison{
	  char* name;
	  int airline;
	  int number;
	  int passengerCount;
	  std::vector<int>baggageCount;
	} info; // Contains name, passenger count, bag count per passenger
};

class CheckInOfficer{
	public:
	  CheckInOfficer(char* deBugName, int i, int y);
	  ~CheckInOfficer();
	  char* getName();
	  void setBreak();
	  bool getBreak(); // For managers to see who is on break
	  void DoWork();
	  int getAirline();
	  int getNumber();
	
	private:
	  struct CheckIn{
		char* name;
		int number;
		int passengerCount;
		std::vector<Baggage> bags;
		int airline;
		bool OnBreak;  
		bool work;
	  }info;
};

class CargoHandler{
	public:
		CargoHandler(int n);
		~CargoHandler();
		int getName();
		bool getBreak(){return onBreak;}
		void DoWork();
		
	private:
		int name;
		bool onBreak;
};
