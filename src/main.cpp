#include <map>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>

#include "strutils.h"
#include "client.h"
#include "verrors.h.h"
#include "cmds.h"
#include "props.h"

#include <ctype.h>

static char* toLower(char* str) {
	for(int i = 0; str[i]; i++){
	  str[i] = std::tolower(str[i]);
	}
	return str;
}

static std::string toLower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(),
    [](unsigned char c){ return std::tolower(c); });
	return str;
}

static char* trim(char *str) {
	char *end;
	while(isspace(*str))
		str++;
	if(*str == 0)
		return str;
	end = str + strnlen(str, 128) - 1;
	while(end > str && isspace(*end))
		end--;
	*(end+1) = '\0';
	return str;
}

int main(int argc, char const* argv[]) {
	srand (time(NULL));
	// Setting config values.
	login_username = (conf.getValue("username", ""));
	login_password = (conf.getValue("password", ""));
	room = (conf.getValue("room", "GroundZero#Reception<dimension-1>"));
	avatar = (conf.getValue("avatar", "avatar:pengo.mov"));
	xPos = conf.getInt("xpos", 0);
	yPos = conf.getInt("ypos", 0);
	zPos = conf.getInt("zpos", 0);
	direction = conf.getInt("direction", 0);
	spin = conf.getInt("spin", 45);
	keepAliveTime = conf.getInt("katime", 15);
	debug = conf.getInt("debug", 0) == 1;
	autoInit();
	while (autoOnline) {
		while (!roomOnline) {}
		sleep(1);
	}
	return deinit(0);
}

int deinit(int response) {
	sendChatMessage(&roomsock, conf.getMessage("goodbye"));
	if (roomOnline) sessExit(&roomsock);
	roomOnline = false;
	if (autoOnline) sessExit(&autosock);
	autoOnline = false;
	if (aRecv_t.joinable()) aRecv_t.detach();
	if (rRecv_t.joinable()) rRecv_t.detach();
	if (rKeepAlive_t.joinable()) rKeepAlive_t.detach();
	close(autosock);
	close(roomsock);
	return response;
}

void autoInit() {
	sockaddr_in auto_addr;
	auto_addr.sin_family = AF_INET;
	memcpy((void*)&auto_addr.sin_addr.s_addr, (void*)&autoserver, sizeof(roomserver));
	auto_addr.sin_port = htons(autoport);
	
	if ((autosock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("AutoSock creation error");
	
	if (connect(autosock, (struct sockaddr*)&auto_addr, sizeof(auto_addr)) < 0)
		perror("AutoServ connection failed");
	
	aRecv_t = std::thread(reciever, &autosock, autoport);
	wsend(&autosock, new unsigned char[] {0x03, 0xff, CMD_PROPREQ}, 0);
	printf("info: Connected to AutoServer: %i.%i.%i.%i:%i\n", autoserver[1], autoserver[1], autoserver[2], autoserver[3], autoport);
	sessInit(&autosock, login_username, login_password);
	autoOnline = true;
}

void roomInit() {
	sockaddr_in room_addr;
	room_addr.sin_family = AF_INET;
	memcpy((void*)&room_addr.sin_addr.s_addr, (void*)&roomserver, sizeof(roomserver));
	room_addr.sin_port = htons(roomport);
	
	if ((roomsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("AutoSock creation error");
	
	if ((connect(roomsock, (struct sockaddr*)&room_addr, sizeof(room_addr))) < 0)
		perror("RoomServ connection failed");
	
	wsend(&autosock, new unsigned char[] {0x03, 0xff, CMD_PROPREQ}, 0);
	printf("info: Connected to RoomServer: %i.%i.%i.%i:%i\n", roomserver[1], roomserver[1], roomserver[2], roomserver[3], roomport);
	roomOnline = true;
	rRecv_t = std::thread(reciever, &roomsock, roomport);
	sessInit(&roomsock, login_username, login_password);
	sleep(1);
	rKeepAlive_t = std::thread(roomKeepAlive);
	rAutoMsg_t = std::thread(autoRandMessage);
}

void roomKeepAlive() {
	while (roomOnline) {
		if (direction >= 360) direction = 0;
		else direction+=spin;
		teleport(&roomsock, xPos, yPos, zPos, direction);
		sleep(keepAliveTime); // So we don't auto-disconnect.
	}
	printf("warning: room keep alive disconnected!\n");
}

void autoRandMessage() {
	int minTime = conf.getInt("minRandomMsgTime", 0);
	int maxTime = conf.getInt("maxRandomMsgTime", 0);
	int wait = 0;
	sleep(2);
	sendChatMessage(&roomsock, conf.getMessage("greet"));
	if (minTime != 0) {
		while (roomOnline) {
			if (wait != 0) {
				std::string newMsg = conf.getMessage("random");
				if (debug) printf("debug: automsg: \"%s\"\n", newMsg.c_str());
				sendChatMessage(&roomsock, newMsg);
			}
			wait = rand() % minTime + maxTime;
			if (debug) printf("debug: waiting %i seconds for next message.\n", wait);
			sleep(wait);
		}
	}
	printf("warning: room auto-messenger disconnected! Ignore if minRandomMsgTime is 0.\n");
}

void reciever(int *sock, uint16_t port) {
	unsigned char bufin[BUFFERSIZE] = {};
	while (read(*sock, bufin, sizeof(bufin)) > 0) {
		int p = 0;
		more_in_buffer:
		int buflen = bufin[p];
		if (buflen == 0x00) continue;
		memset(&(bufout[0]), 0, sizeof(bufout));
		if (bufin[p+2] == CMD_PROPUPD && bufin[p+1] == 0xff) {
			readPropertyList(bufin);
		} else if (bufin[p+2] == CMD_CHATMSG && bufin[p+1] == 0x01) {
			char *username = new char[32];
			char *message = new char[250];
			int offs = p+3;
			offs++; // Ignore empty byte
			int username_len = bufin[offs++];
			memcpy(username, &bufin[offs], username_len);
			username[username_len+1] = 0;
			offs+=username_len;
			int message_len = bufin[offs++];
			memcpy(message, &bufin[offs++], message_len);
			message[message_len+1] = 0;
			printf("info: received message from %s: \"%s\"\n", username, message);
			processText(sock, username, message);
		} else if (bufin[p+2] == CMD_WHISPER && bufin[p+1] == 0x01) {
			char *username = new char[32];
			char *message;
			int offs = p+3;
			offs++; // Ignore empty byte
			int username_len = bufin[offs++];
			memcpy(username, &bufin[offs], username_len);
			username[username_len] = 0;
			offs+=username_len;
			message = new char[250-username_len];
			int message_len = bufin[offs++];
			memcpy(message, &bufin[offs++], message_len);
			message[message_len] = 0;
			printf("info: received whisper from %s: \"%s\"\n", username, message);
			processWhisper(sock, username, message);
		} else if (bufin[p+2] == CMD_REGOBJID && bufin[p+1] == 0xff) {
			char *longID = new char[32];
			int shortID;
			int offs = p+3;
			int long_len = bufin[offs++];
			memcpy(longID, &bufin[offs], long_len);
			longID[long_len] = 0;
			offs+=long_len;
			shortID = bufin[offs++];
			if (longID != NULL || strlen(longID) > 0) {
				Drone newDrone = *(new Drone(longID));
				objects[shortID] = newDrone;
			}
		} else if (bufin[p+2] == CMD_SESSEXIT && bufin[p+1] == 0x01) {
			autoOnline = roomOnline = false;
			deinit(0);
		} else if (bufin[p+2] == CMD_SESSINIT && bufin[p+1] == 0x01) {
			std::map<char, char*> props = readOldPropertyList(&bufin[p]);
			int errNo = VAR_OK;
			for (auto const& p : props)
				switch (p.first) {
					case PROP_ERROR:
						errNo = atoi(p.second);
						break;
				}
			if (errNo != VAR_OK) {
				char* errMsg = "UNKNOWN";
				switch(errNo) {
					case VAR_BAD_PASSWORD:
						errMsg = "Invalid Password."; break;
					case VAR_BAD_ACCOUNT:
						errMsg = "Account is no longer valid."; break;
					case VAR_BAD_IPADDRESS:
						errMsg = "Invalid client IP address!"; break;
					case VAR_NO_SUCH_USER:
						errMsg = "User does not exist."; break;
				}
				printf("error: code %i received: %s\n", errNo, errMsg);
				
				if (port == autoport) {
					autoOnline = false;
				} else {
					roomOnline = false;
				}
			} else {
				if (port != autoport) {
					setAvatar(sock, avatar);
				} else {
					roomIDReq(sock, room);
				}
			}
		} else if ((bufin[p+2] == CMD_ROOMID || bufin[p+2] == 0x1A) && bufin[p+1] == 0x01) {
			char *longID = new char[255];
			int shortID;
			int offs = p+3;
			int long_len = bufin[offs++];
			memcpy(longID, &bufin[offs], long_len);
			longID[long_len+1] = 0;
			offs+=long_len;
			shortID = ((uint16_t)bufin[offs++] << 8) | bufin[offs++];
			roomID = shortID;
			memcpy(&roomserver, new uint8_t[4]{ bufin[offs++], bufin[offs++], bufin[offs++], bufin[offs++] }, 4);
			roomport = (bufin[offs++] << 8) | bufin[offs++];
			if (port != autoport) sessExit(sock);
			printf("info: Joining room %s\n", longID);
			objects.erase(objects.begin(), objects.end());
			roomInit();
			teleport(sock, xPos, yPos, zPos, direction);
		} else if (bufin[p+2] == CMD_ACTOR_DISAPPR && bufin[p+1] == 0xfe) {
			for (int t = 3; t < buflen; t++) {
				userExit(bufin[p+t]);
			}
		} else if (bufin[p+2] == CMD_ACTOR_APPR && bufin[p+1] == 0xfe) {
			for (int t = 3; t < buflen; t+=11) {
				userEnter(bufin[p+t]);
			}
		} else if (bufin[p+2] == CMD_TELEPORT && bufin[p+1] == 0xfe) {
			int offs = p+3;
			int objID = bufin[offs++];
			int exitType = bufin[offs++];
			int entryType = bufin[offs++];
			int roomid = ((uint16_t)bufin[offs++] << 8) | bufin[offs++];
			if (exitType != 0) {
				// De-register ObjID and say user left.
				userExit(objID);
			} else {
				if (entryType == 0) {
					userExit(objID);
				} else {
					userEnter(objID);
				}
			}
		} else {
			//printf("*LEN[%i] OID[%i] TYPE[%02X] OFF[%i]\n", bufin[p], bufin[p+1], bufin[p+2], p);
		}
		if (buflen < sizeof(bufin) && bufin[bufin[p]] != 0x00) {
			p += bufin[p];
			goto more_in_buffer;
		}
		memset(&(bufin[0]), 0, sizeof(bufin));
	}
}

void sessInit(int *sock, std::string username, std::string password) {
	bufout[1] = 0x01;
	bufout[2] = CMD_SESSINIT;
	int l = 3;
	
	// Username
	bufout[l++] = 2;
	bufout[l++] = username.length();
	for (int c = 0; c < username.length(); c++)
		bufout[l++] = username[c];
	
	// Password
	bufout[l++] = 6;
	bufout[l++] = password.length();
	for (int c = 0; c < password.length(); c++)
		bufout[l++] = password[c];
		
	// Protocol
	char* pstr = new char[4];
	sprintf(pstr, "%d", protocol);
	bufout[l++] = 3;
	bufout[l++] = strlen(pstr);
	for (int c = 0; c < strlen(pstr); c++)
		bufout[l++] = pstr[c];
	
	// Avatars
	char* avstr = new char[4];
	sprintf(avstr, "%d", avatars);
	bufout[l++] = 7;
	bufout[l++] = strlen(avstr);
	for (int c = 0; c < strlen(avstr); c++)
		bufout[l++] = avstr[c];
	
	// Version
	bufout[l++] = 9;
	bufout[l++] = strlen(version);
	for (int c = 0; c < strlen(version); c++)
		bufout[l++] = version[c];
		
	// Version
	bufout[l++] = 12;
	bufout[l++] = 1;
	bufout[l++] = '1';
	
	bufout[0] = l;
	bufout[l+1] = 0;
	wsend(sock, bufout, 0);
}

void sessExit(int *sock) {
	bufout[0] = 0x03;
	bufout[1] = 0x01;
	bufout[2] = CMD_SESSEXIT;
	wsend(sock, bufout, 0);
}

void constructPropertyList(int type, std::map<int, char*> props, unsigned char* snd) {
	snd[1] = 0x01;
	snd[2] = type;
	int l = 3;
	for (auto const& p : props) {
		snd[l++] = p.first;
		snd[l++] = strlen(p.second);
		for (int c = 0; c < strlen(p.second); c++)
			snd[l++] = p.second[c];
	}
	snd[0] = l;
	snd[l+1] = 0;
}

void readPropertyList(unsigned char* in) {
	int l {3};
	while (l < in[0]) {
		char property[128] = {0};
		int type = in[l++];
		l++; l++;
		int len = in[l++];
		memcpy(property, &in[l], len);
		property[len]='\0';
		properties[type] = (char*)property;
		l+=len;
	}
}

std::map<char, char*> readOldPropertyList(unsigned char* in) {
	std::map<char, char*> oldprops = {};
	int l {3};
	while (l < in[0]) {
		char* value = new char[255];
		char type = in[l++];
		int len = in[l++];
		memcpy(value, &in[l], len);
		l+=len;
		value[len]=0;
		oldprops[type] = value;
	}
	return oldprops;
}

void setAvatar(int *sock, std::string avstr) {
	unsigned char bufav[255] = {0};
	int l = 1;
	bufav[l++] = 0x01;
	bufav[l++] = CMD_PROPSET;
	bufav[l++] = 0x00;
	bufav[l++] = 0x05;
	bufav[l++] = 0x40;
	bufav[l++] = 0x01;
	
	bufav[l++] = avstr.length();
	for (int c = 0; c < avstr.length(); c++)
		bufav[l++] = avstr[c];
	
	bufav[0] = l;
	bufav[l+1] = 0;
	wsend(sock, bufav, 0);
}

void roomIDReq(int *sock, std::string room) {
	unsigned char bufrm[255] = {0};
	bufrm[1] = 1;
	bufrm[2] = CMD_ROOMIDREQ;
	bufrm[3] = room.length();
	int x = 4;
	for (int z = 0; z < room.length(); z++)
		bufrm[x++] = room[z];
	bufrm[x++] = 0;
	bufrm[0] = x;
	wsend(sock, bufrm, 0);
}

void teleport(int *sock, int x, int y, int z, int rot) {
	unsigned char buftp[16] = {0};
	uint8_t _roomID[2];
	uint8_t _x[2];
	uint8_t _y[2];
	uint8_t _z[2];
	uint8_t _rot[2];
	memcpy(_roomID, &roomID, sizeof(_roomID));
	memcpy(_x, &x, sizeof(_x));
	memcpy(_y, &y, sizeof(_y));
	memcpy(_z, &z, sizeof(_z));
	memcpy(_rot, &rot, sizeof(_rot));
	
	buftp[0] = 0x0f;
	buftp[1] = 0x01;
	buftp[2] = CMD_TELEPORT;
	buftp[4] = _roomID[0]; buftp[3] = _roomID[1];
	buftp[5] = 0x00; buftp[6] = 0x01;
	buftp[8] = _x[0]; buftp[7] = _x[1];
	buftp[10] = _y[0]; buftp[9] = _y[1];
	buftp[12] = _z[0]; buftp[11] = _z[1];
	buftp[14] = _rot[0]; buftp[13] = _rot[1];
	
	wsend(sock, buftp, 0);
}

void userEnter(char id) {
	if (!((Drone)objects[id]).droneActive) {
		objects[id].droneActive = true;
	}
}

void userExit(char id) {
	if (((Drone)objects[id]).droneActive) {
		objects.erase((char)id);
	}
}


bool strcontains(std::string needle, std::string haystack) {
	bool found = haystack.find(needle) != std::string::npos;
	if (debug) printf("debug: %s =?= %s == %i\n", needle.c_str(), haystack.c_str(), found?1:0);
	return found;
}

bool vstrcontains(std::string needle, std::vector<std::string> haystack) {
	for (std::string str : haystack) {
		if (needle.rfind(str, 0) == 0) return true;
	}
	return false;
}

std::string getContainedWorld(std::map<std::string, std::string> worldlist, std::string input) {
	for (auto world : worldlist) {
		if (strcontains(world.first, input)) return world.second;
	}
	return "";
}

char* handleCommand(std::string from, std::string message) {
	char *msgout = new char[255];
	if (strcontains("flip a coin", message)) {
		
		bool heads = rand() % 10 > 5;
		snprintf(msgout, 255, "%s", heads?"Heads":"Tails");
		
	} else if (strcontains("time", message)) {
		
		time_t currentTime;
		struct tm *localTime;
		time( &currentTime );
		localTime = localtime( &currentTime );
		snprintf(msgout, 255, "The time is %02i:%02i.", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
		
	} else if (strcontains("dice", message) || strcontains("roll", message)) {
		
		int dice = 6;
		
		if (strcontains("d10", message)) dice = 10;
		else if (strcontains("d12", message)) dice = 12;
		else if (strcontains("d20", message)) dice = 20;
		else if (strcontains("d100", message)) dice = 100;
		
		int roll = (rand() % (dice-1)) + 1;
		snprintf(msgout, 255, "%s rolled a %i.", from.c_str(), dice);
		
	} else if (strcontains("where is", message) || strcontains("where", message) || strcontains("mark to", message) || strcontains("show me", message)) {
		
		std::string mark = getContainedWorld(conf.getWorlds(), message);
		if (mark.length() > 0) {
			snprintf(msgout, 255, conf.getMessage("whereis").c_str(), mark.c_str());
		} else {
			snprintf(msgout, 255, "Sorry! Not a clue.");
		}
		
	} else if (strcontains("many users", message) || strcontains("whos online", message) || strcontains("who is online", message) || strcontains("many online", message)) {
		
		snprintf(msgout, 255, "There are %i users in this room.", objects.size());
		
	} else if ((strcontains("what", message) || strcontains("who", message)) && strcontains("are you", message)) {
		
		snprintf(msgout, 255, "%s", conf.getMessage("whoami").c_str());
		
	} else if (strcontains("shutdown", message) && from == conf.getValue("owner", "")) {
		
		exit(deinit(0));
		
	}
	return msgout;
}

void processText(int *sock, std::string username, std::string message) {
	char *msgout = new char[255];
	message = toLower(message); // Make it a lowercase string so we can work with it.
	// Someone has requested P3NG0s attention.
	// We'll accept some variations.
	if (username != login_username) {
		if (vstrcontains(message, conf.getMessages("attention"))) {
			msgout = handleCommand(username, message);
			if (strlen(msgout) > 0) sendChatMessage(sock, msgout);
			else if (strcontains("your commands", message) || strcontains("can you do", message)) {

				char *whisout = new char[255];
				snprintf(whisout, 255, "Check this out: %s", conf.getValue("help_url", "").c_str());
				sendWhisperMessage(&roomsock, username, whisout);
				snprintf(msgout, 255, "You have been whispered with a link that will help you.");
				sendChatMessage(sock, msgout);

			}
		} else if (message == "ping") { // We'll accept a simple ping to pong.
			sendChatMessage(sock, "Pong!");
		}
	}
}

void processWhisper(int *sock, std::string username, std::string message) {
	char *msgout = new char[255];
	message = toLower(message);
	
	if (message == "ping") {
		sendWhisperMessage(sock, username, "Pong");
	} else {
		msgout = handleCommand(username, message);
		if (strlen(msgout) > 0) sendWhisperMessage(sock, username, msgout);
	}
}

void sendChatMessage(int *sock, std::string msg) {
	unsigned char bufout[BUFFERSIZE] = {0};
	int msglen = msg.length();
	int k = 1;
	bufout[k++] = 1;
	bufout[k++] = CMD_CHATMSG;
	bufout[k++] = 0; bufout[k++] = 0;
	bufout[k++] = msglen;
	for(int l = 0; l < msglen; l++)
		bufout[k++] = msg[l];
	bufout[k] = 0;
	bufout[0] = k;
	wsend(&roomsock, bufout, 0);
}

void sendWhisperMessage(int *sock, std::string to, std::string msg) {
	whisper_repeat:
	unsigned char bufout[BUFFERSIZE] = {0};
	int k = 1;
	bufout[k++] = 0;
	bufout[k++] = to.length();
	for(int l = 0; l < to.length(); l++)
		bufout[k++] = to[l];
	bufout[k++] = CMD_WHISPER;
	bufout[k++] = 0; bufout[k++] = 0;
	int msglen = std::min((int)msg.length(), 226);
	bufout[k++] = msglen;
	for(int l = 0; l < msglen; l++)
		bufout[k++] = msg[l];
	bufout[k] = 0;
	bufout[0] = k;
	wsend(&roomsock, bufout, 0);
	// Check if the length is higher.
	if (msg.length() > 226) {
		msg = msg.substr(226, -1);
		goto whisper_repeat;
	}
}

int wsend(int *sock, unsigned char str[], int flags) {
	if (debug) printf("debug: sending new packet of type %i and length %i.\n", str[2], str[0]);
	return send(*sock, str, str[0], flags);
}
