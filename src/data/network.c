#include "network.h"

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

char* buildAPIRequest(SongData songInfo) {
    char* sanitisedStrings[4];

    char* outputURL = NULL;

    char* requestParameterStringsInArray[3] = {
        songInfo.metadata->artist,
        songInfo.metadata->title,
        songInfo.metadata->album
    };
    char* requestParameterFieldsArray[3] = {"artist_name=", "track_name=", "album_name="};

    int requestLength = strlen(ROOT_URL);

    for (int i = 0; i < 3; i++) {
        if (requestParameterStringsInArray[i] != NULL) {
            sanitisedStrings[i] = sanitiseStringForURL(requestParameterStringsInArray[i],
                                                       strlen(requestParameterStringsInArray[i]),
                                                       requestParameterFieldsArray[i]);
            requestLength += strlen(sanitisedStrings[i]);
            // printf("%s\n", sanitisedStrings[i]);
        }
        else sanitisedStrings[i] = NULL;
    }
    
    if (songInfo.duration == 0) return NULL;

    // 1 for the ampersand and 1 for the NULL terminator
    int durationStringSize = strlen(DURATION_PARAMETER) + 1 + numberOfDigits(songInfo.duration) + 1;

    sanitisedStrings[3] = calloc(durationStringSize, sizeof(char));
    if (sanitisedStrings[3] == NULL) return NULL;
    
    snprintf(sanitisedStrings[3], durationStringSize, "&%s%f", DURATION_PARAMETER, songInfo.duration);
    requestLength += strlen(sanitisedStrings[3]);

    outputURL = calloc(requestLength + 1, sizeof(char));
    if (outputURL == NULL) return NULL;

    strncat(outputURL, ROOT_URL, strlen(ROOT_URL) + 1);

    for (int i = 0; i < 4; i++) {
        if (sanitisedStrings[i] != NULL) {
            strncat(outputURL, sanitisedStrings[i], strlen(sanitisedStrings[i]) + 1);
            free(sanitisedStrings[i]);
        }
    }

    // printf("%s\n", outputURL);

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
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respStr);
        curl_easy_setopt(curl, CURLOPT_URL, inputURL);

        result = curl_easy_perform(curl);
        if(result != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(result));
            return NULL;
        }

        curl_easy_cleanup(curl);
    }
    else return NULL;
    
    curl_global_cleanup();

    return respStr;
}
