#ifndef H_DRONE
#define H_DRONE

class Drone {
public:
	bool droneActive = false;
	char* name = new char[24];
	
	Drone() {  };
	Drone(char* &_name) : name{ _name } { };
	bool operator != (Drone &d) { return strcmp(this->name, d.name) != 0; };
	Drone operator = (Drone *d) { return *d; };
};

#endif