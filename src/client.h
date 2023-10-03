#ifndef H_CLIENT
#define H_CLIENT
#include <vector>
#include <thread>
#include <fstream>
#include <stdlib.h>
#include "drone.h"
bool debug = false;
std::random_device rd;
std::mt19937 rng(rd());

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
				if (debug) std::cout << name << ": \"" << value << "\"" << std::endl;
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
						if (debug) std::cout << msgName << "=" << line << std::endl;
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
					if (debug) std::cout << name << ": \"" << value << "\"" << std::endl;
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
					if (debug) std::cout << name << ": \"" << value << "\"" << std::endl;
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
		std::uniform_int_distribution<int> rno(0,(messages[type]).size()-1);
		return (messages[type])[rno(rng)];
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

PengoConfig conf;

#define BUFFERSIZE 65535

bool autoOnline = false;
bool roomOnline = false;

unsigned char bufout[BUFFERSIZE] = {0};
int randWait;

int autosock;
int roomsock;

std::thread aRecv_t;
std::thread rRecv_t;
std::thread rKeepAlive_t;
std::thread rAutoMsg_t;

uint8_t autoserver[4] = { 209, 240, 84, 122 };
uint16_t autoport = 6650;
uint8_t roomserver[4] = { 209, 240, 84, 122 };
uint16_t roomport = 5672;

std::string login_username;
std::string login_password;
std::string avatar = "avatar:pengo.mov";

// Needs to include dimension too
std::string room;
uint16_t roomID = 1;

int protocol = 24;
char* version = "1000000000";
int avatars = 253;
int keepAliveTime;

uint16_t xPos = 0;
uint16_t yPos = 0;
uint16_t zPos = 0;
uint16_t direction = 0;
uint16_t spin = 0;

std::map<char, char*> properties;
std::map<char, Drone> objects;

static char* toLower(char* str);
static std::string toLower(std::string str);
static char* trim(char *str);
void loadConfig();
int deinit(int response);
void autoInit();
void roomInit();
void roomKeepAlive();
void autoRandMessage();
void reciever(int *sock, uint16_t port);
void sessInit(int *sock, std::string username, std::string password);
void sessExit(int *sock);
void constructPropertyList(int type, std::map<int, char*> props, unsigned char* snd);
void readPropertyList(unsigned char* in);
std::map<char, char*> readOldPropertyList(unsigned char* in);
void setAvatar(int *sock, std::string avstr);
void roomIDReq(int *sock, std::string room);
void teleport(int *sock, int x, int y, int z, int rot);
void userEnter(char id);
void userExit(char id);
bool strcontains(std::string needle, std::string haystack);
int vstrcontains(std::string needle, std::vector<std::string> haystack);
std::string getContainedWorld(std::map<std::string, std::string> worldlist, std::string input);
std::string getResponse(std::map<std::string, std::string> responselist, std::string input);
char* handleCommand(std::string from, std::string message);
void processText(int *sock, std::string username, std::string message);
void processWhisper(int *sock, std::string username, std::string message);
void sendChatMessage(int *sock, std::string msg);
void sendWhisperMessage(int *sock, std::string to, std::string msg);
int wsend(int *sock, unsigned char str[], int flags);

#endif