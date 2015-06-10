#include "copyright.h"
#include "system.h"
#include "synch.h"
#include <list>

#define LIAISONLINE_COUNT 5

void SimpleThread(int which);
void ThreadTest();
int liaisonLine[LIAISONLINE_COUNT];
Condition *liaisonLineCV[LIAISONLINE_COUNT];
Lock *liaisonLineLock;
Lock *liaisonLineLock[LIAISONLINE_COUNT];
LiaisonOfficer *liaisonOfficers[LIAISONLINE_COUNT];

class Passenger {
  public:
	  Passenger(int n);
	  ~Passenger();
	  int getName() { return name;}			// debugging assist
	  int getAirline() {return airline;}
	  int getTicket() {return economy;}
	  void ChooseLiaisonLine();
	  int getBaggageCount() {return baggageCount;}
      void setAirline(int n);
	  
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
    LiaisonOfficer();
	~LiaisonOfficer();
	int getName();
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
}
