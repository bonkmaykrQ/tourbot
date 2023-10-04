#include <map>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <cmath>
#include <iomanip>
#include <random>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "strutils.h"
#include "client.h"
#include "verrors.h"
#include "cmds.h"
#include "props.h"

std::chrono::time_point<std::chrono::system_clock> pingStart;
std::chrono::time_point<std::chrono::system_clock> pingEnd;

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
	// Setting config values.
	if (argc > 1) {
		confFile = std::string(argv[1]);
		printf("info: primary conf file set to \"%s\"\n", confFile.c_str());
	}
	loadConfig();
	/*for (auto const& c : mainConf->get()) {
		printf("info: (%s),(%s)\n", c.first.c_str(), c.second.c_str());
	}*/
	autoInit();
	while (autoOnline) {
		while (!roomOnline) {}
		sleep(1);
	}
	return deinit(0);
}

void loadConfig() {
	mainConf = new ACFile(confFile.c_str());
	worldlist = new ACFile(mainConf->getValue("worldfile", "conf/worldlist.conf").c_str());
	replylist = new ACFile(mainConf->getValue("replyfile", "conf/replies.conf").c_str());
	messages = new AMFile(mainConf->getValue("messages", "conf/messages.list").c_str());
	login_username = mainConf->getValue("username", "");
	login_password = mainConf->getValue("password", "");
	room = mainConf->getValue("room", "GroundZero#Reception<dimension-1>");
	avatar = mainConf->getValue("avatar", "avatar:pengo.mov");
	xPos = mainConf->getInt("xpos", 0);
	yPos = mainConf->getInt("ypos", 0);
	zPos = mainConf->getInt("zpos", 0);
	direction = mainConf->getInt("direction", 0);
	spin = mainConf->getInt("spin", 0);
	keepAliveTime = mainConf->getInt("katime", 15);
	debug = mainConf->getInt("debug", 0) == 1;
}

int deinit(int response) {
	sendChatMessage(&roomsock, messages->getMessage("goodbye"));
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
	int minTime = mainConf->getInt("minRandomMsgTime", 0);
	int maxTime = mainConf->getInt("maxRandomMsgTime", 0);
	int wait = 0;
	sleep(2);
	sendChatMessage(&roomsock, messages->getMessage("startup"));
	if (minTime != 0) {
		while (roomOnline) {
			if (wait != 0) {
				std::string newMsg = messages->getMessage("random");
				if (debug) printf("debug: automsg: \"%s\"\n", newMsg.c_str());
				sendChatMessage(&roomsock, newMsg);
			}
			std::uniform_int_distribution<int> rno(minTime,maxTime);
			wait = rno(rng);
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
			username[username_len] = 0;
			offs+=username_len;
			int message_len = bufin[offs++];
			memcpy(message, &bufin[offs++], message_len);
			message[message_len] = 0;
			processText(sock, std::string(username), std::string(message));
		} else if (bufin[p+2] == CMD_WHISPER && bufin[p+1] == 0x01) {
			char *username = new char[32];
			char *message = new char[250];
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
			processWhisper(sock, std::string(username), std::string(message));
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

int vstrcontains(std::string needle, std::vector<std::string> haystack) {
	for (std::string str : haystack) {
		if (needle.rfind(str, 0) == 0) return str.length();
	}
	return 0;
}

std::string getContainedWorld(std::map<std::string, std::string> worldlist, std::string input) {
	for (auto world : worldlist) {
		if (strcontains(world.first, input)) return world.second;
	}
	return "";
}

std::string getResponse(std::map<std::string, std::string> responselist, std::string input) {
	for (auto resp : responselist) {
		if (strcontains(resp.first, input)) return resp.second;
	}
	return "";
}

char* handleCommand(std::string from, std::string message) {
	char *msgout = new char[255];
	
	if (strcontains("flip a coin", message)) {
		
		std::uniform_int_distribution<int> rno(0,1);
		bool heads = rno(rng) == 1;
		snprintf(msgout, 255, mainConf->getValue("coinflip_msg", "%s flipped %s.").c_str(), heads?"heads":"tails");
		
	} else if (strcontains("time", message)) {
		
		std::ostringstream nowStream;
		auto now = std::chrono::system_clock::now();
		std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
		nowStream << std::put_time(localtime(&nowTime), "%A %b %d, %Y at %I:%M %p %Z");
		std::string curTime = nowStream.str();
		
		snprintf(msgout, 255, mainConf->getValue("time_msg", "It is %s.").c_str(), curTime.c_str());
		
	} else if (strcontains("roll", message)) {
		
		int dice = 6; int dstr;
		try {
			if ((dstr = message.find(" d")) != std::string::npos) {
				std::string dino = message.substr(dstr+2, message.length());
				if (debug) printf("debug: setting die to d%s\n", dino.c_str());
				dice = std::stoi(dino);
			}
		} catch (const std::out_of_range& e) { 
			dice = 6;
		}
		
		std::uniform_int_distribution<int> rno(1,dice);
		int roll = rno(rng);
		snprintf(msgout, 255, mainConf->getValue("roll_msg", "%s rolled a %i.").c_str(), from.c_str(), roll);
		
	} else if (strcontains("where is", message) || strcontains("where", message) || strcontains("mark to", message) || strcontains("show me", message)) {
		
		std::string mark = getContainedWorld(worldlist->get(), message);
		if (mark.length() > 0) {
			snprintf(msgout, 255, messages->getMessage("world").c_str(), mark.c_str());
		} else {
			snprintf(msgout, 255, mainConf->getValue("world_not_found_msg", "Sorry, I don't know that one.").c_str());
		}
		
	} else if (((strcontains("many", message) || strcontains("count", message)) && (strcontains("users", message) || strcontains("people", message))) || (strcontains("online", message) && (strcontains("who", message) || strcontains("how many", message) || strcontains("who is", message)))) {
		
		snprintf(msgout, 255, mainConf->getValue("roomusers_msg", "There are %i users in this room.").c_str(), objects.size());
		
	} else if ((strcontains("joke", message) && strcontains("tell", message)) || (strcontains("make", message) && strcontains("laugh", message))) {
		
		snprintf(msgout, 255, messages->getMessage("jokes").c_str());
		
	} else if ((strcontains("what", message) || strcontains("who", message)) && strcontains("are you", message)) {
		
		snprintf(msgout, 255, messages->getMessage("whoami").c_str());
		
	} else if (strcontains("shutdown", message) && from == mainConf->getValue("owner", "")) {
		
		exit(deinit(0));
		
	} else if (strcontains("reload", message) && from == mainConf->getValue("owner", "")) {
		
		loadConfig();
		snprintf(msgout, 255, mainConf->getValue("conf_reload_msg", "My configurations have been reloaded.").c_str());
		
	} else if (strcontains(" hi", message) || strcontains("hello", message) || strcontains(" hey", message) || strcontains(" yo", message)) {
		// Prefix small ones with spaces so they don't get mixed up easily in other words. "this" -> hi, "you" -> yo, "they" -> hey
		
		snprintf(msgout, 255, messages->getMessage("greets").c_str(), from.c_str());
		
	} else {
		
		std::string response;
		if ((response = getResponse(replylist->get(), message)).length() > 0) {
			snprintf(msgout, 255, response.c_str());
		}
		
	}
	
	return msgout;
}

void processText(int *sock, std::string username, std::string message) {
	printf("info: received text from %s: \"%s\"\n", username.c_str(), message.c_str());
	char *msgout = new char[255];
	if (username.compare(login_username) != 0) {
		message = toLower(message); // Make it a lowercase string so we can work with it.
		int alen = 0;
		// Someone has requested P3NG0s attention.
		// We'll accept some variations.
		if ((alen = vstrcontains(message, messages->getMessages("attention"))) > 0) {
			// Strip out the attention. We got it.
			message = message.substr(alen+1, message.length());
			msgout = handleCommand(username, message);
			if (strlen(msgout) > 1) sendChatMessage(sock, std::string(msgout));
			else if (strcontains("your commands", message) || strcontains("can you do", message)) {

				char *whisout = new char[255];
				snprintf(whisout, 255, mainConf->getValue("help_msg", "You can find more information here: %s").c_str(), mainConf->getValue("help_url", "").c_str());
				sendWhisperMessage(&roomsock, username, whisout);
				snprintf(msgout, 255, mainConf->getValue("help_whisper_message", "%s, you have been whispered with more information.").c_str(), username.c_str());
				sendChatMessage(sock, msgout);

			} else if (strcontains("ping test", message)) {

				pingStart = std::chrono::system_clock::now();
				snprintf(msgout, 255, "Ping!");
				sendChatMessage(sock, msgout);

			}
		} else if (message == "ping") { // We'll accept a simple ping to pong.
			sendChatMessage(sock, mainConf->getValue("pong_msg", "Pong!"));
		}
	} else {
		if (debug) printf("debug: processing own message: %s\n", message.c_str());
		if (message.compare("Ping!") == 0) {
			pingEnd = std::chrono::system_clock::now();
			std::chrono::duration<double, std::milli> ping = pingEnd - pingStart;
			snprintf(msgout, 255, mainConf->getValue("ping_msg", "Response recieved in %ims.").c_str(), ping.count());
			sendChatMessage(sock, msgout);
		}
	}
}

void processWhisper(int *sock, std::string username, std::string message) {
	printf("info: received whisper from %s: \"%s\"\n", username.c_str(), message.c_str());
	char *msgout = new char[255];
	message = toLower(message);
	
	if (message == "ping") {
		sendWhisperMessage(sock, username, mainConf->getValue("pong_msg", "Pong!"));
	} else {
		msgout = handleCommand(username, message);
		if (strlen(msgout) > 0) sendWhisperMessage(sock, username, msgout);
		else {
			snprintf(msgout, 255, messages->getMessage("unknown").c_str());
			sendWhisperMessage(sock, username, msgout);
		}
	}
}

void sendChatMessage(int *sock, std::string msg) {
	if (msg.length() > 0) {
		unsigned char msgout[255] = {0};
		int msglen = msg.length();
		int k = 1;
		msgout[k++] = 1;
		msgout[k++] = CMD_CHATMSG;
		msgout[k++] = 0; bufout[k++] = 0;
		msgout[k++] = msglen;
		for(int l = 0; l < msglen; l++)
			msgout[k++] = msg[l];
		msgout[k] = 0;
		msgout[0] = k;
		wsend(&roomsock, msgout, 0);
	}
}

void sendWhisperMessage(int *sock, std::string to, std::string msg) {
	if (msg.length() > 0 && to.length() > 0) {
		whisper_repeat:
		unsigned char msgout[255] = {0};
		int k = 1;
		msgout[k++] = 0;
		msgout[k++] = to.length();
		for(int l = 0; l < to.length(); l++)
			msgout[k++] = to[l];
		msgout[k++] = CMD_WHISPER;
		msgout[k++] = 0; bufout[k++] = 0;
		int msglen = std::min((int)msg.length(), 226);
		msgout[k++] = msglen;
		for(int l = 0; l < msglen; l++)
			msgout[k++] = msg[l];
		msgout[k] = 0;
		msgout[0] = k;
		wsend(&roomsock, msgout, 0);
		// Check if the length is higher.
		if (msg.length() > 226) {
			msg = msg.substr(226, -1);
			goto whisper_repeat;
		}
	}
}

int wsend(int *sock, unsigned char str[], int flags) {
	if (debug) printf("debug: sending new packet of type %i and length %i.\n", str[2], str[0]);
	return send(*sock, str, str[0], flags);
}
