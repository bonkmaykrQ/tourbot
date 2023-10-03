#ifndef H_CLIENT
#define H_CLIENT
#include <vector>
#include <thread>
#include "drone.h"
#include "config.h"
PengoConfig conf;
bool debug = false;

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

int deinit(int response);
static char* trim(char *str);
char* zero(int size);
unsigned char* uzero(int size);
void autoInit();
void roomInit();
void roomKeepAlive();
void autoRandMessage();
void reciever(int *sock, uint16_t port);
void sessInit(int *sock, std::string username, std::string password);
void sessExit(int *sock);
void readPropertyList(unsigned char* in);
std::map<char, char*> readOldPropertyList(unsigned char* in);
void setAvatar(int *sock, std::string avatar);
void roomIDReq(int *sock, std::string room);
void teleport(int *sock, int x, int y, int z, int rot);
char* dimAdd(std::string room);
bool strcontains(std::string needle, std::string haystack);
bool vstrcontains(std::string needle, std::vector<std::string> haystack);
void processText(int *sock, std::string username, std::string message);
void processWhisper(int *sock, std::string username, std::string message);
void sendChatMessage(int *sock, std::string msg);
void sendWhisperMessage(int *sock, std::string to, std::string msg);
int wsend(int *sock, unsigned char str[], int flags);

#endif