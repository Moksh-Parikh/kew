#ifndef KEW_NETWORK
#define KEW_NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "lyrics.h"
#include "stringFunctions.h"

#define ASCII_GROUP_1_START 0x21
#define ASCII_GROUP_1_END   0x2f
#define ASCII_GROUP_2_START 0x3a
#define ASCII_GROUP_2_END   0x3f
#define ASCII_GROUP_3       0x40
#define ASCII_GROUP_4       0x5b
#define ASCII_GROUP_5       0x5d

#define ROOT_URL            "https://lrclib.net/api/get?"
#define DURATION_PARAMETER  "duration="

char* sanitiseStringForURL(char* inString, int inStringLength, char* parameterPrefix);
char* buildAPIRequest(SongData songInfo);
size_t write_to_string(void *contents, size_t size, size_t nmemb, void *userp);
char* curlRequest(char* inputURL);

#endif
