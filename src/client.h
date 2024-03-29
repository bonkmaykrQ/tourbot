#ifndef H_CLIENT
#define H_CLIENT
#include <vector>
#include <thread>
#include <fstream>
#include <stdlib.h>
#include "drone.h"
#include "group.h"
#include "acfile.h"
#define BUFFERSIZE 4096

bool debug = true;
std::random_device rd;
std::mt19937 rng(rd());

const std::vector<std::string> blacklist = {
	"worldsmaster",
	"trade",
	"worlds"
};

const std::string user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36";
std::string confFile = "conf/bot.conf";
ACFile* mainConf;
ACFile* worldlist;
ACFile* marksLUT;
ACFile* replylist;
AMFile* messages;
ALFile* filth;
ALFile* nogroup;

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
std::string avatar = "http://files.worlio.com/users/bonkmaykr/avatarsforme/gabu1s*4h*4v*.mov";

// Needs to include dimension too
std::string room;
uint16_t roomID = 1;

int protocol = 24;
char* version = "1000000000";
int avatars = 253; // avoid culling users if possible... what could go wrong?
int keepAliveTime;

uint16_t xPos = 0;
uint16_t yPos = 0;
uint16_t zPos = 0;
uint16_t direction = 0;
uint16_t spin = 0;

std::map<char, char*> properties;
std::map<char, Drone*> objects;
std::vector<Group> groups;

namespace internalTypes {
	struct dronePositionI {
		int x;
		int y;
		int z;
		int yaw;
	};

	struct dronePositionF {
		float x;
		float y;
		float z;
		float yaw;
	};

	struct markEntry {
		std::string name;
		std::string url;
		std::string room;
		dronePositionI position;
		bool blacklist;
	};
}

void loadConfig();
int deinit(int response);
void autoInit();
void roomInit();
void roomKeepAlive();
void autoRandMessage();
void reciever(int *sock, uint16_t port, bool* status);
void sessInit(int *sock, std::string username, std::string password);
void sessExit(int *sock);
void constructPropertyList(int type, std::map<int, char*> props, unsigned char* snd);
void readPropertyList(unsigned char* in);
std::map<char, char*> readOldPropertyList(unsigned char* in);
void setAvatar(int *sock, std::string avstr);
void roomIDReq(int *sock, std::string room);
void teleport(int *sock, int x, int y, int z, int rot);
void longloc(int *sock, int x, int y, int z, int rot);
void setBuddy(int* sock, std::string name, bool status);
void userEnter(char id);
void userExit(char id);
Group* findGroupOfMember(std::string name);
bool handleCommand(char* buffer, std::string from, std::string message);
bool handlePhrase(char* buffer, std::string from, std::string message);
void processText(int *sock, std::string username, std::string message);
void processWhisper(int *sock, std::string username, std::string message);
void sendChatMessage(int *sock, std::string msg);
void sendWhisperMessage(int *sock, std::string to, std::string msg);
void relayGroupMessage(Group* g, int *sock, std::string from, std::string text);
void sendGroupMessage(Group* g, int *sock, std::string message);
void safeDeleteGroupMember(Group* g, std::string member);
void qsend(int *sock, unsigned char str[], bool queue);

void handleTeleportRequest(internalTypes::markEntry details);
void handleTour(std::string destination[4]);
void lookUpWorldName(std::string alias, char* buffer);

Drone* getDrone(std::string name) {
	for (auto o : objects)
		if (o.second->name == name) return o.second;
	return nullptr;
}

#endif