#ifndef CONFIG_H
#define CONFIG_H
#include <fstream>
#include <vector>
#include <stdlib.h>

class PengoConfig {
public:
	PengoConfig() {
		std::ifstream mconf("conf/pengobot.conf");
		
		std::string line;
		while (std::getline(mconf, line)) {
			// Check if line is a comment line.
			if (line.rfind("#", 0) != 0) {
				std::string name = line.substr(0, line.find("="));
				std::string value = line.substr(line.find("=")+1, line.length());
				std::cout << name << ": \"" << value << "\"" << std::endl;
				conf[name] = value;
			}
		}
		
		for (auto const& mc : conf) {
			std::string msgName;
			if ((mc.first.substr(0, mc.first.find("_"))) == "msg") {
				msgName = mc.first.substr(mc.first.find("_")+1, mc.first.length());
				std::ifstream msglines(mc.second);
				std::string line;
				std::vector<std::string> lines;
				while (std::getline(msglines, line)) {
					if (line.rfind("#", 0) != 0) {
						lines.push_back(line);
						std::cout << msgName << "=" << line << std::endl;
					}
				}
				messages[msgName] = lines;
			}
		}
		
		if ((conf["worldfile"]).length() > 0) {
			std::ifstream wrlds(conf["worldfile"]);
			std::string wline;
			while (std::getline(wrlds, wline)) {
				if (wline.rfind("#", 0) != 0) {
					std::string name = wline.substr(0, wline.find("="));
					std::string value = wline.substr(wline.find("=")+1, wline.length());
					std::cout << name << ": \"" << value << "\"" << std::endl;
					worlds[name] = value;
				}
			}
		}
		
		if ((conf["responsefile"]).length() > 0) {
			std::ifstream rspn(conf["responsefile"]);
			std::string rline;
			while (std::getline(rspn, rline)) {
				if (rline.rfind("#", 0) != 0) {
					std::string name = rline.substr(0, rline.find("="));
					std::string value = rline.substr(rline.find("=")+1, rline.length());
					std::cout << name << ": \"" << value << "\"" << std::endl;
					responses[name] = value;
				}
			}
		}
	}
	PengoConfig operator = (PengoConfig *pc) { return *pc; };
	
	std::string getValue(std::string name, std::string def) {
		if (((std::string)conf[name]).length() > 0) return conf[name];
		else return def;
	}
	
	int getInt(std::string name, int def) {
		if (((std::string)conf[name]).length() > 0) return std::stoi(conf[name]);
		else return def;
	}
	
	void setValue(std::string name, std::string value) {
		conf[name] = value;
	}
	
	void setInt(std::string name, int value) {
		conf[name] = std::to_string(value);
	}
	
	std::string getMessage(std::string type) {
		return (messages[type])[(int)(rand() % ((messages[type]).size()))];
	}
	
	std::vector<std::string> getMessages(std::string type) {
		return messages[type];
	}
	
	std::string getWorld(std::string name) {
		return worlds[name];
	}
	
	std::map<std::string, std::string> getWorlds() {
		return worlds;
	}
	
	std::string getReply(std::string name) {
		return responses[name];
	}
	
	std::map<std::string, std::string> getResponses() {
		return responses;
	}
	
private:
	std::map<std::string, std::string> conf;
	std::map<std::string, std::string> worlds;
	std::map<std::string, std::string> responses;
	std::map<std::string, std::vector<std::string>> messages;
};

#endif

