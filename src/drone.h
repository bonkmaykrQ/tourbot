#ifndef H_DRONE
#define H_DRONE
#include "group.h"

class Drone {
public:
	bool droneActive = false;
	std::string name;
	
	Drone() {  };
	Drone(char* &_name) {
		this->name = std::string(_name);
	};
	Drone(std::string &_name) {
		this->name = _name;
	};
	bool operator != (Drone &d) { return this->name != d.name; };
	Drone operator = (Drone *d) { return *d; };
};

#endif