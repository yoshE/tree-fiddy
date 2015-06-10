#include "copyright.h"
#include "system.h"
#include "synch.h"
#include <list>
#include <vector>

#define LIAISONLINE_COUNT 5
#define CHECKIN_COUNT 5

void SimpleThread(int which);
void ThreadTest();
int liaisonLine[LIAISONLINE_COUNT];
Condition *liaisonLineCV[LIAISONLINE_COUNT];
Condition *CheckIn1CV[CHECKIN_COUNT];
Condition *CheckIn2CV[CHECKIN_COUNT];
Condition *CheckIn3CV[CHECKIN_COUNT];
Lock *liaisonLineLock;
Lock *liaisonLineLocks[LIAISONLINE_COUNT];
Lock *CheckInLock;

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
	  void ChooseCheckIn();
	  
  private:
	  int name;        // useful for debugging
	  int airline;		//which airline does the passenger fly
	  bool economy;		//economy or executive ticket
	  int myLine;		//which line the passenger got into
	  int baggageCount;
	  int baggageWeight[3];
};

class LiaisonOfficer {
  public:
    LiaisonOfficer(char* deBugName);
	~LiaisonOfficer();
	char getName();
	int getPassengerCount(); // For manager to get passenger headcount
	int getPassengerBaggageCount(int n); // For manager to get passenger bag count
	void setPassengerBaggageCount(int n); // Increments passenger count and adds their baggage count to vector
	void PassengerLeaving();
  
  private:
	struct Liaison{
	  char* name;
	  int airline;
	  int passengerCount;
	  std::vector<int>baggageCount;
	}; // Contains name, passenger count, bag count per passenger
};

class CheckInOfficer{
	public:
	  CheckInOfficer(char* deBugName);
	  ~CheckInOfficer();
	  char getName();
	  bool getBreak(); // For managers to see who is on break
	
	private:
	  struct CheckIn{
		char* name;
		int passengerCount;
		std::vector<int>baggageCount;
		int airline;
		bool OnBreak;  
	  };
};
