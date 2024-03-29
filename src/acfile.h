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
		printf("info: reading file %s\n", path);
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
	
	void write(const char* path) {
		std::ofstream outfile(path);
		printf("info: writing to file %s\n", path);
		for (std::map<std::string, std::string>::iterator it = conf.begin(); it != conf.end(); ++it) {
			outfile << it->first << "=" << it->second << std::endl;
		}
		outfile.close();
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
		printf("info: reading file %s\n", path);
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
	
	void write(const char* path) {
		std::ofstream outfile(path);
		printf("info: writing to file %s\n", path);
		for (std::map<std::string, std::vector<std::string>>::iterator mi = messages.begin(); mi != messages.end(); ++mi) {
			outfile << "[" << mi->first << "]" << std::endl;
			for (auto vi = mi->second.begin(); vi < mi->second.end(); mi++) {
				outfile << *vi << std::endl;
			}
		}
		outfile.close();
	}
private:
	std::map<std::string, std::vector<std::string>> messages;
	std::random_device rd;
	std::mt19937 rng;
};

// Another List File
class ALFile {
public:
	ALFile() {};
	ALFile(const ALFile &other) {
		this->list = other.list;
	}
	ALFile(const char* path) {
		std::ifstream infile(path);
		printf("info: reading file %s\n", path);
		std::string line;
		while (std::getline(infile, line)) {
			if (line.length() == 0) continue;
			// Check if line is a group defining line.
			if (line.rfind("#", 0) != 0) {
				list.push_back(line);
			}
		}
	}
	
	std::vector<std::string> getLines() {
		return list;
	}
	
	void addLine(std::string entry) {
		list.push_back(entry);
	}
	
	void delLine(std::string entry) {
		std::vector<std::string>::iterator p = std::find(list.begin(), list.end(), entry);
		if (p != list.end()) {
			list.erase(p);
		};
	}
	
	void write(const char* path) {
		std::ofstream outfile(path);
		printf("info: writing to file %s\n", path);
		for (std::string value : list) {
			outfile << value << std::endl;
		}
		outfile.close();
	}
private:
	std::vector<std::string> list;
};

#endif

