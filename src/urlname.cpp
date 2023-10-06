#include <iostream>
#define CURL_STATICLIB
#include <curl/curl.h>
#include <math.h>
#include <unistd.h>
#ifdef __linux__
    #include <sys/ioctl.h>
#endif



size_t my_array_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
	
}

void getTitle(std::string url, const char* filename) {
	
}