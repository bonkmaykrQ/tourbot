#ifndef GROUP_H
#define GROUP_H
#include <vector>
#include <map>
#include "drone.h"
#include "client.h"

class Group {
public:
	std::vector<std::string> members;
	
	Group() {};
	Group(const Group &other) {
		this->members = other.members;
	}
	
	bool addMember(std::string user) {
		if (std::find(members.begin(), members.end(), user) == members.end()) {
			members.push_back(user);
			return true;
		}
		return false;
	}
	
	bool delMember(std::string user) {
		std::vector<std::string>::iterator p = std::find(members.begin(), members.end(), user);
		if (p != members.end()) {
			members.erase(p);
			if (members.size() == 0) delete this;
			return true;
		}
		return false;
	}
};

#endif

