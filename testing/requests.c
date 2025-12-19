#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define ASCII_GROUP_1_START 0x21
#define ASCII_GROUP_1_END   0x2f
#define ASCII_GROUP_2_START 0x3a
#define ASCII_GROUP_2_END   0x3f
#define ASCII_GROUP_3       0x40
#define ASCII_GROUP_4       0x5b
#define ASCII_GROUP_5       0x5d

#define ROOT_URL            "https://lrclib.net/api/get?"
#define DURATION_PARAMETER  "duration="

#define CURL_BEGINNING      "curl -s"

typedef struct {
    char* artist_name;
    char* track_name;
    char* album_name;
    int duration;
} requestStruct;

char* sanitiseStringForURL(char* inString, int inStringLength, char* parameterPrefix) {
    char* tempString = calloc(3 * inStringLength, sizeof(char));
    int tempStringSize;
    char* output;
    char* escapedCharacterBuffer = calloc(4, sizeof(char));

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
    // absolute spaghetti will optimise later
    char* sanitisedArtist = NULL;
    char* sanitisedTrack = NULL;
    char* sanitisedAlbum = NULL;
    char* outputURL = NULL;
    char* durationString = NULL;

    int requestLength = strlen(ROOT_URL);

    if (requestParameters.artist_name != NULL) {
        sanitisedArtist = sanitiseStringForURL(requestParameters.artist_name, strlen(requestParameters.artist_name), "artist_name=");
        requestLength += strlen(sanitisedArtist);
    }

    if (requestParameters.track_name != NULL) {
        sanitisedTrack = sanitiseStringForURL(requestParameters.track_name, strlen(requestParameters.track_name), "track_name=");
        requestLength += strlen(sanitisedTrack);
    }

    if (requestParameters.album_name != NULL) {
        sanitisedAlbum = sanitiseStringForURL(requestParameters.album_name, strlen(requestParameters.album_name), "album_name=");
        requestLength += strlen(sanitisedAlbum);
    }
    
    if (requestParameters.duration != 0) {
        // 1 for the ampersand and 1 for the NULL terminator
        int durationStringSize = strlen(DURATION_PARAMETER) + 1 + numberOfDigits(requestParameters.duration) + 1;
        durationString = calloc(durationStringSize, sizeof(char));
        snprintf(durationString, durationStringSize, "&%s%d", DURATION_PARAMETER, requestParameters.duration);
        requestLength += strlen(durationString);
    }

    outputURL = calloc(requestLength, sizeof(char));

    strncat(outputURL, ROOT_URL, strlen(ROOT_URL) + 1);

    if (sanitisedArtist != NULL) {
        strncat(outputURL, sanitisedArtist, strlen(sanitisedArtist));
        free(sanitisedArtist);
    }

    if (sanitisedAlbum != NULL) {
        strncat(outputURL, sanitisedAlbum, strlen(sanitisedAlbum));
        free(sanitisedAlbum);
    }

    if (sanitisedTrack != NULL) {
        strncat(outputURL, sanitisedTrack, strlen(sanitisedTrack));
        free(sanitisedTrack);
    }

    if (durationString != NULL) {
        strncat(outputURL, durationString, strlen(durationString));
        free(durationString);
    }

    return outputURL;
}

char* curlRequest(char* inputURL) {
    FILE* curlOutput;
    char* outBuffer = NULL;

    int tempBufferSize = 1024;
    int outputBufferSize = 8192;

    char tempBuffer[tempBufferSize];
    char outputBuffer[outputBufferSize];

    int commandSize = strlen(CURL_BEGINNING) + strlen(inputURL) + 2 + 1 + 1; // 2 for the single quotes around URL and 1 for the space and 1 for the NULL

    char* curlCommand = calloc(commandSize, sizeof(char));
    snprintf(curlCommand, commandSize, "%s '%s'", CURL_BEGINNING, inputURL);

    curlOutput = popen(curlCommand, "r");
    free(curlCommand);

    while (fgets(tempBuffer, sizeof(tempBuffer), curlOutput) != NULL) {
        if (strlen(outputBuffer) + strlen(tempBuffer) >= outputBufferSize) {
            fprintf(stderr, "Output buffer overflow\n");
            pclose(curlOutput);
            return NULL;
        }
        strcat(outputBuffer, tempBuffer);
    }

    pclose(curlOutput);

    outBuffer = calloc(strlen(outputBuffer) + 1, sizeof(char));
    strncpy(outBuffer, outputBuffer, strlen(outputBuffer));

    return outBuffer;
}

int main(int argc, char **argv) {
    char* response;
    char* request = buildAPIRequest((requestStruct){"Linkin Park", "Somewhere I Belong", "Meteora", 213});

    response = curlRequest(request);
    free(request);

    printf("%s\n", response);

    free(response);

    return 0;
}
