#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>

template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

bool strcontains(std::string needle, std::string haystack) {
	bool found = haystack.find(needle) != std::string::npos;
	return found;
}

bool vfind(std::vector<std::string> v, std::string i) {
	return std::find(v.begin(), v.end(), i) != v.end();
}

int vstrcontains(std::string needle, std::vector<std::string> haystack) {
	for (std::string str : haystack) {
		if (needle.rfind(str, 0) == 0) return str.length();
	}
	return 0;
}

std::string getValueOfIncludedName(std::map<std::string, std::string> list, std::string input) {
	for (auto v : list) {
		if (strcontains(v.first, input)) return v.second;
	}
	return "";
}

std::string strcombine(std::vector<std::string> args, int start, int end, char pad) {
	std::string output = args[start];
	for (int c = start+1; c < end; c++) {
		output+=(pad+args[c]);
	}
	return output;
}

static char* toLower(char* str) {
	for(int i = 0; str[i]; i++){
	  str[i] = std::tolower(str[i]);
	}
	return str;
}

static std::string toLower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
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

#endif

