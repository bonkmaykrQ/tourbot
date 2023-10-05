#ifndef ACF_H
#define ACF_H
#include <fstream>
#include <vector>
#include <stdlib.h>

// Another Configuration File
class ACFile {
public:
	ACFile() {};
	ACFile(const ACFile &other) {
		this->conf = other.conf;
		for (auto const& c : this->conf) {
			printf("processed: (%s),(%s)\n", c.first.c_str(), c.second.c_str());
		}
	}
	ACFile(const char* path) {
		std::ifstream infile(path);
		
		std::string line;
		while (std::getline(infile, line)) {
			if (line.length() == 0) continue;
			// Check if line is a comment line.
			if (line.rfind("#", 0) != 0) {
				std::string name = line.substr(0, line.find("="));
				std::string value = line.substr(line.find("=")+1, line.length());
				conf[name] = value;
			}
		}
	}
	
	std::map<std::string, std::string> get() {
		for (auto const& c : this->conf) {
			printf("info: (%s),(%s)\n", c.first.c_str(), c.second.c_str());
		}
		return this->conf;
	}
	
	std::vector<std::string> getKeyValues() {
		std::vector<std::string> values;
		for (auto const& c : conf) {
			values.push_back(c.first);
		}
		return values;
	}
	
	std::string getValue(std::string name, std::string def) {
		if (conf.find(name) != conf.end()) return conf[name];
		else return def;
	}
	
	int getInt(std::string name, int def) {
		if (conf.find(name) != conf.end()) return std::stoi(conf[name]);
		else return def;
	}
	
	void setValue(std::string name, std::string value) {
		conf[name] = value;
	}
	
	void setInt(std::string name, int value) {
		conf[name] = std::to_string(value);
	}
private:
	std::map<std::string, std::string> conf;
};

// Another Message File
class AMFile {
public:
	AMFile() {};
	AMFile(const AMFile &other) {
		this->messages = other.messages;
	}
	AMFile(const char* path) {
		this->rng = std::mt19937(rd());
		
		std::ifstream infile(path);
		
		std::string line; std::string group;
		while (std::getline(infile, line)) {
			if (line.length() == 0) continue;
			// Check if line is a group defining line.
			if (line.rfind("[", 0) == 0) {
				group = line.substr(1, line.find("]")-1);
				messages[group] = {};
			} else if (line.rfind("#", 0) != 0) {
				if (line.rfind("\\", 0) == 0) {
					line = line.substr(1, line.length());
				} // For \[text] messages.
				messages[group].push_back(line);
			}
		}
	}
	
	std::map<std::string, std::vector<std::string>> get() {
		return messages;
	}
	
	std::string getMessage(std::string group) {
		if (messages.find(group) == messages.end() || messages[group].size() == 0) return "";
		std::uniform_int_distribution<int> rno(0,(messages[group]).size()-1);
		return (messages[group])[rno(rng)];
	}
	
	std::vector<std::string> getMessages(std::string group) {
		if (messages.find(group) == messages.end()) return {};
		return messages[group];
	}
private:
	std::map<std::string, std::vector<std::string>> messages;
	std::random_device rd;
	std::mt19937 rng;
};

#endif

