#ifndef GROUP_H
#define GROUP_H
#include <vector>
#include <map>

class Group {
public:
	Group() {};
	Group(const Group &other) {
		this->owner = other.owner;
		this->members = other.members;
	}
	Group(std::string owner) {
		this->owner = owner;
	}
	
	void addMember(std::string name) {
		members.push_back(name);
	}
	
	void delMember(std::string name) {
		members.erase(std::find(members.begin(), members.end(), name));
	}
private:
	std::string owner;
	std::vector<std::string> members;
};

std::vector<Group> groups;
std::map<std::string, Group*> groupmap;

#endif

