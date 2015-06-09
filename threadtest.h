#include "copyright.h"
#include "system.h"
#include "synch.h"
#include <list>

#define LIAISONLINE_COUNT 5

void SimpleThread(int which);
void ThreadTest();
int liaisonLine[LIAISONLINE_COUNT];
//std::list<Condition> *liaisonLineCV;
Condition liaisonLineCV[LIAISONLINE_COUNT];
Lock *liaisonLineLock;

class Passenger {
  public:
	  Passenger(int n);
	  ~Passenger();
	  int getName() { return name;}			// debugging assist
	  int getAirline() {return airline;}
	  int getTicket() {return economy;}
	  void ChooseLiaisonLine();
    
  private:
	  int name;        // useful for debugging
	  int airline;		//which airline does the passenger fly
	  bool economy;		//economy or executive ticket
	  int myLine;		//which line the passenger got into
	  int baggageCount;
	  int baggageWeight[3];
};
