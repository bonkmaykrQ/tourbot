#ifndef H_CLIENT
#define H_CLIENT
#include <vector>
#include <thread>
#include "drone.h"
#include "config.h"
PengoConfig conf;
bool debug = false;
std::random_device rd;
std::mt19937 rng(rd());

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