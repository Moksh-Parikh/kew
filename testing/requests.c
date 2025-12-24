#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>

#include <curl/curl.h>

#define ASCII_GROUP_1_START 0x21
#define ASCII_GROUP_1_END   0x2f
#define ASCII_GROUP_2_START 0x3a
#define ASCII_GROUP_2_END   0x3f
#define ASCII_GROUP_3       0x40
#define ASCII_GROUP_4       0x5b
#define ASCII_GROUP_5       0x5d

#define ROOT_URL            "https://lrclib.net/api/get?"
#define DURATION_PARAMETER  "duration="

typedef struct {
    char* artist_name;
    char* track_name;
    char* album_name;
    int duration;
} requestStruct;

char* sanitiseStringForURL(char* inString, int inStringLength, char* parameterPrefix) {
    char* tempString = calloc(3 * inStringLength, sizeof(char));
    if (tempString == NULL) return NULL;
    
    int tempStringSize;
    char* output;
    char escapedCharacterBuffer[4];

    strncat(tempString, "&", 2);
    strncat(tempString, parameterPrefix, strlen(parameterPrefix) + 1);

    for (int i = 0; i < inStringLength; i++) {
        char currentCharacter = inString[i];

        if ( (currentCharacter >= ASCII_GROUP_1_START && currentCharacter <= ASCII_GROUP_1_END) ||
             (currentCharacter >= ASCII_GROUP_2_START && currentCharacter <= ASCII_GROUP_2_END && currentCharacter != '<') || // left angle bracket doesn't need to be escaped
             (currentCharacter == ASCII_GROUP_3) ||
             (currentCharacter == ASCII_GROUP_4) ||
             (currentCharacter == ASCII_GROUP_5)
        ) {
            snprintf(escapedCharacterBuffer, 4, "%c%X", '%', currentCharacter);
            strncat(tempString, escapedCharacterBuffer, 3);
        }
        else if (currentCharacter == ' ') {
            strncat(tempString, "+", 2);
        }
        else {
            strncat(tempString, inString + i, 1);
        }
    }

    tempStringSize = strlen(tempString) + 1;
    output = calloc(tempStringSize, sizeof(char));
    if (output == NULL) return NULL;

    strncpy(output, tempString, tempStringSize);
    free(tempString);

    return output;
}

// cheers https://stackoverflow.com/questions/1068849/how-do-i-determine-the-number-of-digits-of-an-integer-in-c
int numberOfDigits (int n) {
    int r = 1;
    if (n < 0) n = (n == INT_MIN) ? INT_MAX: -n;
    while (n > 9) {
        n /= 10;
        r++;
    }
    return r;
}

char* buildAPIRequest(requestStruct requestParameters) {
    char* sanitisedStrings[4];

    char* outputURL = NULL;

    char* requestParameterStringsInArray[3] = {
        requestParameters.artist_name,
        requestParameters.track_name,
        requestParameters.album_name
    };
    char* requestParameterFieldsArray[3] = {"artist_name=", "track_name=", "album_name="};

    int requestLength = strlen(ROOT_URL);

    for (int i = 0; i < 3; i++) {
        if (requestParameterStringsInArray[i] != NULL) {
            sanitisedStrings[i] = sanitiseStringForURL(requestParameterStringsInArray[i],
                                                       strlen(requestParameterStringsInArray[i]),
                                                       requestParameterFieldsArray[i]);
            requestLength += strlen(sanitisedStrings[i]);
        }
        else sanitisedStrings[i] = NULL;
    }
    
    if (requestParameters.duration != 0) {
        // 1 for the ampersand and 1 for the NULL terminator
        int durationStringSize = strlen(DURATION_PARAMETER) + 1 + numberOfDigits(requestParameters.duration) + 1;
        
        sanitisedStrings[3] = calloc(durationStringSize, sizeof(char));
        if (sanitisedStrings[3] == NULL) return NULL;
        
        snprintf(sanitisedStrings[3], durationStringSize, "&%s%d", DURATION_PARAMETER, requestParameters.duration);
        requestLength += strlen(sanitisedStrings[3]);
    }

    outputURL = calloc(requestLength + 1, sizeof(char));
    if (outputURL == NULL) return NULL;

    strncat(outputURL, ROOT_URL, strlen(ROOT_URL) + 1);

    for (int i = 0; i < 4; i++) {
        if (sanitisedStrings[i] != NULL) {
            strncat(outputURL, sanitisedStrings[i], strlen(sanitisedStrings[i]) + 1);
            free(sanitisedStrings[i]);
        }
    }

    return outputURL;
}

// Inspired by filthy ChatGPT code
size_t write_to_string(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    char **buffer = (char **)userp;
    char *tempPointer;

    if (contents == NULL || real_size == 0) {
        return 0;
    }

    tempPointer = calloc(real_size + 1, sizeof(char));
    if (tempPointer == NULL) {
        return 0;
    }

    strncpy(tempPointer, (char *)contents, real_size);
    tempPointer[real_size] = '\0';
    *buffer = tempPointer;

    return real_size;
}

char* curlRequest(char* inputURL) {
    CURL *curl;
 
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    if(result)
        return NULL;

    char* respStr = NULL;

    curl = curl_easy_init();
    
    char* outBuffer = NULL;

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respStr);
        curl_easy_setopt(curl, CURLOPT_URL, inputURL);

        result = curl_easy_perform(curl);
        if(result != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(result));

        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();

    return respStr;
}

// this function only needs to find strings to pull the syncedLyrics field from the API response,
// so it will not get the content of a field if it is an integer or boolean, such as the duration and instrumental fields
char* jsonParser(char* jsonString, char* requestedField) {
    if (jsonString == NULL || requestedField == NULL) return NULL;

    char* fieldContent = NULL;
    int index = 0;

    char* currentSearchPosition = jsonString;
    while (currentSearchPosition < (jsonString + strlen(jsonString) ) ) {
        currentSearchPosition = strstr(currentSearchPosition, requestedField);
        if (currentSearchPosition == NULL) break;

        index = currentSearchPosition - jsonString;

        char* maybeSecondDoubleQuote = currentSearchPosition + strlen(requestedField);

        if ( *(currentSearchPosition - 1) == '"' &&
             *maybeSecondDoubleQuote == '"'
        ) {
            char* firstContentDoubleQuote = strchr(maybeSecondDoubleQuote + 1, '"');
            if (firstContentDoubleQuote == NULL) {
                return NULL;
            }

            char* secondContentDoubleQuote = strchr(firstContentDoubleQuote + 1, '"');
            if (secondContentDoubleQuote == NULL) {
                return NULL;
            } else if (*(secondContentDoubleQuote - 1) == '\\') {
                for (int i = 0; i < strlen(jsonString); i++) {
                    secondContentDoubleQuote = strchr(secondContentDoubleQuote + 1, '"');
                    if (*(secondContentDoubleQuote - 1) != '\\') break;
                    if (secondContentDoubleQuote == NULL) return NULL;
                }
            }


            int contentSize = secondContentDoubleQuote - firstContentDoubleQuote - 1;
            fieldContent = calloc((contentSize + 1), sizeof(char));

            strncpy(fieldContent, firstContentDoubleQuote + 1, contentSize);
            return fieldContent;
        }

        currentSearchPosition++;
    }
}

bool LRCExists(char* originalFilePath) {
    char lrcPath[1024];
    if (snprintf(lrcPath, sizeof(lrcPath), "%s", originalFilePath) >= (int)sizeof(lrcPath))
        return NULL;

    char *dot = strrchr(lrcPath, '.');
    if (!dot || dot == lrcPath)
        return NULL;

    if (snprintf(dot, sizeof(lrcPath) - (dot - lrcPath), ".lrc") >= (int)(sizeof(lrcPath) - (dot - lrcPath)))
        return NULL;

    return access(lrcPath, F_OK);
}

int main(int argc, char **argv) {
    char* response;
    char* request = buildAPIRequest((requestStruct){"Linkin Park", "Somewhere I Belong", "Meteora", 213});

    response = curlRequest(request);
    free(request);

    char* syncedLyrics = jsonParser(response, "syncedLyrics");
    if (syncedLyrics != NULL) {
        printf("%s\n", syncedLyrics);
    }

    free(response);
    free(syncedLyrics);

    printf("%d\n", LRCExists("john.lrc"));

    return 0;
}
