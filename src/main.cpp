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

#define CURL_STATICLIB
#include <curl/curl.h>

#include "client.h"
#include "verrors.h"
#include "cmds.h"
#include "props.h"
#include "group.h"
#include "utils.h"

class QueuedPacket {
public:
	QueuedPacket() {};
	QueuedPacket(const QueuedPacket &other) {
		this->sock = other.sock;
		memcpy(this->buf, other.buf, other.buf[0]);
	}
	QueuedPacket(int *sock, unsigned char str[]) {
		this->sock = *sock;
		memcpy(this->buf, str, str[0]);
	};
	
	int flush(int flags) {
		return send(this->sock, this->buf, this->buf[0], flags);
	}
private:
	int sock;
	unsigned char buf[255] = {};
};

std::vector<QueuedPacket> bufferQueue;

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
		if (bufferQueue.size() > 0) {
			if (debug) printf("debug: flushing buffer queue (%i packets)\n", bufferQueue.size());
			for (QueuedPacket qp : bufferQueue) {
				qp.flush(MSG_MORE);
			}
			send(autosock, new unsigned char[]{}, 0, 0);
			send(roomsock, new unsigned char[]{}, 0, 0);
			bufferQueue.clear();
		}
	}
	return deinit(0);
}

void loadConfig() {
	mainConf = new ACFile(confFile.c_str());
	worldlist = new ACFile(mainConf->getValue("worldfile", "conf/worldlist.conf").c_str());
	replylist = new ACFile(mainConf->getValue("replyfile", "conf/replies.conf").c_str());
	messages = new AMFile(mainConf->getValue("messages", "conf/messages.list").c_str());
	filth = new ALFile(mainConf->getValue("filthlist", "conf/filth.list").c_str());
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
	send(autosock, new unsigned char[] {0x03, 0xff, CMD_PROPREQ}, 3, 0);
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
	
	send(autosock, new unsigned char[] {0x03, 0xff, CMD_PROPREQ}, 3, 0);
	printf("info: Connected to RoomServer: %i.%i.%i.%i:%i\n", roomserver[1], roomserver[1], roomserver[2], roomserver[3], roomport);
	roomOnline = true;
	rRecv_t = std::thread(reciever, &roomsock, roomport);
	sessInit(&roomsock, login_username, login_password);
	sleep(1);
	rKeepAlive_t = std::thread(roomKeepAlive);
	sleep(1);
	rAutoMsg_t = std::thread(autoRandMessage);
}

void roomKeepAlive() {
	sleep(1);
	teleport(&roomsock, xPos, yPos, zPos, direction);
	while (roomOnline) {
		if (direction >= 360) direction += (spin - 360);
		else direction+=spin;
		longloc(&roomsock, xPos, yPos, zPos, direction);
		sleep(keepAliveTime);
	}
	printf("warning: room keep alive disconnected!\n");
}

void autoRandMessage() {
	int minTime = mainConf->getInt("minRandomMsgTime", 0);
	int maxTime = mainConf->getInt("maxRandomMsgTime", 0);
	int wait = 0;
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
		memset(&bufin[0], 0, sizeof(bufin));
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
	qsend(sock, bufout, false);
}

void sessExit(int *sock) {
	bufout[0] = 0x03;
	bufout[1] = 0x01;
	bufout[2] = CMD_SESSEXIT;
	qsend(sock, bufout, false);
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
	qsend(sock, bufav, true);
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
	qsend(sock, bufrm, false);
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
	buftp[8] = _x[1]; buftp[7] = _x[0];
	buftp[10] = _y[1]; buftp[9] = _y[0];
	buftp[12] = _z[1]; buftp[11] = _z[0];
	buftp[14] = _rot[1]; buftp[13] = _rot[0];
	
	qsend(sock, buftp, false);
}

void longloc(int *sock, int x, int y, int z, int rot) {
	unsigned char buftp[11] = {0};
	uint8_t _x[2];
	uint8_t _y[2];
	uint8_t _z[2];
	uint8_t _rot[2];
	memcpy(_x, &x, sizeof(_x));
	memcpy(_y, &y, sizeof(_y));
	memcpy(_z, &z, sizeof(_z));
	memcpy(_rot, &rot, sizeof(_rot));
	
	buftp[0] = 0x0b;
	buftp[1] = 0x01;
	buftp[2] = CMD_LONGLOC;
	buftp[3] = _x[1]; buftp[4] = _x[0];
	buftp[5] = _y[1]; buftp[6] = _y[0];
	buftp[7] = _z[1]; buftp[8] = _z[0];
	buftp[9] = _rot[1]; buftp[10] = _rot[0];
	
	qsend(sock, buftp, true);
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

bool handleCommand(char* buffer, std::string from, std::string message) {
	std::vector<std::string> args = split(message, ' ');
	if (args.size() > 0) {
		if (args[0] == "roll" && args.size() > 1) {
			int dice = 1; int sides = 6; int roll = 0;
			int darg = 1;
			if (args[1] == "a") darg = 2;
			if (args.size() > darg) {
				try {
					int dinx = args[darg].find("d");
					if (dinx > 0) {
						dice = std::stoi(args[darg].substr(0, dinx));
					}
					sides = std::stoi(args[darg].substr(dinx+1, args[darg].length()));
				} catch (...) { }
			}

			std::uniform_int_distribution<int> rno(1,sides);
			for (int d = 0; d < dice; d++) roll+=rno(rng);
			sprintf(buffer, mainConf->getValue("roll_msg", "%s rolled a %i.").c_str(), from.c_str(), roll);
			return true;
		} else if ((args[0] == "where" || args[0] == "whereis") && args.size() > 1) {
			std::string mark = getValueOfIncludedName(worldlist->get(), strcombine(args, 1, args.size(), ' '));
			if (mark.length() > 0) {
				sprintf(buffer, messages->getMessage("world").c_str(), mark.c_str());
			} else {
				sprintf(buffer, mainConf->getValue("world_not_found_msg", "Sorry, I don't know that one.").c_str());
			}
			return true;
		} else if (args[0] == "help") {
			char *whisout = new char[255];
			sprintf(whisout, mainConf->getValue("help_msg", "You can find more information here: %s").c_str(), mainConf->getValue("help_url", "").c_str());
			sendWhisperMessage(&roomsock, from, whisout);
			sprintf(buffer, mainConf->getValue("help_whisper_message", "%s, you have been whispered with more information.").c_str(), from.c_str());
			return true;
		} else if (args[0] == "ping") {
			sprintf(buffer, mainConf->getValue("pong_msg", "Pong!").c_str());
			return true;
		} else if (args[0] == "time") {
			std::ostringstream nowStream;
			auto now = std::chrono::system_clock::now();
			std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
			nowStream << std::put_time(localtime(&nowTime), "%A %b %d, %Y at %I:%M %p %Z");
			std::string curTime = nowStream.str();

			sprintf(buffer, mainConf->getValue("time_msg", "It is %s.").c_str(), curTime.c_str());
			return true;
		} else if (args[0] == "stats" || args[0] == "status") {
			sprintf(buffer, mainConf->getValue("roomusers_msg", "There are %i users in this room.").c_str(), objects.size());
			return true;
		} else if (args[0] == "reload" && from == mainConf->getValue("owner", "")) {
			loadConfig();
			setAvatar(&roomsock, avatar);
			sprintf(buffer, mainConf->getValue("conf_reload_msg", "My configurations have been reloaded.").c_str());
			return true;
		} else if ((args[0] == "shutdown" || args[0] == "kill") && from == mainConf->getValue("owner", "")) {
			exit(deinit(0));
			return true;
		}
	}
	return false;
}

bool handlePhrase(char* buffer, std::string from, std::string message) {
	std::vector<std::string> args = split(message, ' ');
	if (strcontains("flip a coin", message)) {
		
		std::uniform_int_distribution<int> rno(0,1);
		bool heads = rno(rng) == 1;
		sprintf(buffer, mainConf->getValue("coinflip_msg", "%s flipped %s.").c_str(), from.c_str(), heads?"heads":"tails");
		return true;
		
	} else if ((strcontains("joke", message) && strcontains("tell", message)) || (strcontains("make", message) && strcontains("laugh", message))) {
		
		sprintf(buffer, messages->getMessage("jokes").c_str());
		return true;
		
	} else if ((strcontains("what", message) || strcontains("who", message)) && strcontains("are you", message)) {
		
		sprintf(buffer, messages->getMessage("whoami").c_str());
		return true;
		
	} else if (vfind(args, "hi") || vfind(args, "hello") || vfind(args, "hey") || vfind(args, "yo")) {
		
		sprintf(buffer, messages->getMessage("greets").c_str(), from.c_str());
		return true;
		
	}
	return false;
}

void processText(int *sock, std::string username, std::string message) {
	if (debug) printf("debug: received text from %s: \"%s\"\n", username.c_str(), message.c_str());
	char *msgout = new char[BUFFERSIZE];
	if (username.compare(login_username) != 0) {
		// We'll make a lowercase version so we can work with it without worrying about cases.
		std::string lowermsg = toLower(message); // Assign message to lowermsg to make a copy.
		int alen = 0;
		// Someone has requested P3NG0s attention.
		// We'll accept some variations.
		if ((alen = vstrcontains(lowermsg, messages->getMessages("attention"))) > 0) {
			// Strip out the attention. We got it.
			if (handleCommand(msgout, username, message.substr(alen+1, message.length()))) {
				printf("info: processed command\n");
			} else if (handlePhrase(msgout, username, lowermsg.substr(alen+1, lowermsg.length()))) {
				printf("info: processed phrase\n");
			}
			sendChatMessage(sock, std::string(msgout));
		} else if (lowermsg == "ping") {
			sprintf(msgout, mainConf->getValue("pong_msg", "Pong!").c_str());
			sendChatMessage(sock, std::string(msgout));
		} else if (lowermsg.find("http") != std::string::npos) {
			int pos;
			if ((pos = lowermsg.find("http://")) != std::string::npos || (pos = lowermsg.find("https://")) != std::string::npos) {
				std::string url = message.substr(pos, message.substr(pos, message.length()).find(" "));
				if (debug) printf("debug: getting title for url \"%s\"\n", url.c_str());
				if(CURL* curl = curl_easy_init()) {
					std::string httpContents;

					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
					curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
					curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
					curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpContents);

					CURLcode res = curl_easy_perform(curl);
					if(res != CURLE_OK) fprintf(stderr, "Error<cURL>: curl_easy_perform() failed > %s\n", curl_easy_strerror(res));
					curl_easy_cleanup(curl);
					if (httpContents.length() > 0 && httpContents.find("<title>") != std::string::npos) {
						std::string linktitle = httpContents.substr(httpContents.find("<title>")+7, httpContents.length());
						linktitle = decodeHTML(filter(filth->getLines(), linktitle.substr(0, linktitle.find("<"))));
						if (linktitle.length() > 0) {
							if (linktitle.length() > 180) linktitle = linktitle.substr(0, 180)+"...";
							if (debug) printf("debug: got title \"%s\"\n", linktitle.c_str());
							sprintf(msgout, mainConf->getValue("link_msg", "[%s] Link: %s").c_str(), username.c_str(), linktitle.c_str());
							sendChatMessage(sock, std::string(msgout));
						}
					}
				}
			}
		}
	}
}

void processWhisper(int *sock, std::string username, std::string message) {
	if (debug) printf("debug: received whisper from %s: \"%s\"\n", username.c_str(), message.c_str());
	char *msgout = new char[255];
	message = toLower(message);
	
	if (message == "ping") {
		sendWhisperMessage(sock, username, mainConf->getValue("pong_msg", "Pong!"));
	} else {
		if (handleCommand(msgout, username, message)) {
			printf("info: processed command\n");
		} else if (handlePhrase(msgout, username, message)) {
			printf("info: processed phrase\n");
		} else {
			snprintf(msgout, 255, messages->getMessage("unknown").c_str());
		}
		sendWhisperMessage(sock, username, msgout);
	}
}

void sendChatMessage(int *sock, std::string msg) {
	if (msg.length() > 0) {
		unsigned char msgout[255] = {0};
		for (size_t i = 0; i < msg.size(); i += 226) {
			const std::string line = msg.substr(i, 226);
			int k = 1;
			msgout[k++] = 1;
			msgout[k++] = CMD_CHATMSG;
			msgout[k++] = 0; bufout[k++] = 0;
			uint8_t msglen = line.length();
			msgout[k++] = msglen;
			for(int l = 0; l < msglen; l++)
				msgout[k++] = line[l];
			msgout[k] = 0;
			msgout[0] = k;
			if(debug) printf("debug: sending text \"%s\"\n", line.c_str());
			unsigned char bufmsg[255];
			for(int l = 0; l < sizeof(msgout); l++)
				bufmsg[l] = msgout[l];
			qsend(sock, bufmsg, true);
			memset(&msgout[0], 0, sizeof(msgout));
		}
	}
}

void sendWhisperMessage(int *sock, std::string to, std::string msg) {
	if (msg.length() > 0 && to.length() > 0) {
		unsigned char msgout[255] = {0};
		for (size_t i = 0; i < msg.size(); i += 226) {
			const std::string line = msg.substr(i, 226);
			int k = 1;
			msgout[k++] = 0;
			msgout[k++] = to.length();
			for(int l = 0; l < to.length(); l++)
				msgout[k++] = to[l];
			msgout[k++] = CMD_WHISPER;
			msgout[k++] = 0; bufout[k++] = 0;
			uint8_t msglen = line.length();
			msgout[k++] = msglen;
			for(int l = 0; l < msglen; l++)
				msgout[k++] = line[l];
			msgout[k] = 0;
			msgout[0] = k;
			if(debug) printf("debug: sending whisper \"%s\"\n", line.c_str());
			qsend(sock, *(&msgout), true);
			memset(&msgout[0], 0, sizeof(msgout));
		}
	}
}

void qsend(int *sock, unsigned char str[], bool queue) {
	if (debug) printf("debug: queuing packet type %i with length %i.\n", str[2], str[0]);
	QueuedPacket* qp = new QueuedPacket(sock, str);
	if (queue) bufferQueue.push_back(*qp);
	else qp->flush(0);
}

