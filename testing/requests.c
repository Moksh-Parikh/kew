#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

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

#define METADATA_MAX_LENGTH 64

typedef struct {
        double timestamp;
        char *text;
} LyricsLine;

typedef struct {
        LyricsLine *lines;
        size_t count;
        int max_length;
        int isTimed;
} Lyrics;

typedef struct
{
        char title[METADATA_MAX_LENGTH];
        char artist[METADATA_MAX_LENGTH];
        char album_artist[METADATA_MAX_LENGTH];
        char album[METADATA_MAX_LENGTH];
        char date[METADATA_MAX_LENGTH];
        double replaygainTrack;
        double replaygainAlbum;
} TagSettings;

typedef struct
{
        int magic;
        // gchar *track_id;
        char file_path[PATH_MAX];
        char cover_art_path[PATH_MAX];
        unsigned char red;
        unsigned char green;
        unsigned char blue;
        TagSettings *metadata;
        unsigned char *cover;
        int avg_bit_rate;
        int coverWidth;
        int coverHeight;
        double duration;
        bool hasErrors;
        Lyrics *lyrics;
} SongData;

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
int numberOfDigits (double n) {
    double tempInt;
    int i = 0, temp = 0;
    
    for (i; i < 3000; i++) {
        if (modf(n, &tempInt) == 0.0f) break;
        n *= 10;
    }
    temp = (int)n;

    int r = 1;
    if (temp < 0) temp = (temp == INT_MIN) ? INT_MAX: -temp;
    while (temp > 9) {
        temp /= 10;
        r++;
    }
    return r + 1; // for the decimal point
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
            printf("%s\n", sanitisedStrings[i]);
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

    printf("%s\n", outputURL);

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
        if(result != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(result));
            return NULL;
        }

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

    char* currentSearchPosition = jsonString;
    while (currentSearchPosition < (jsonString + strlen(jsonString) ) ) {
        currentSearchPosition = strstr(currentSearchPosition, requestedField);
        if (currentSearchPosition == NULL) break;

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

    return !access(lrcPath, F_OK);
}

static int loadTimedLyrics(FILE *file, Lyrics *lyrics) {
        size_t capacity = 64;
        lyrics->lines = (LyricsLine *)malloc(sizeof(LyricsLine) * capacity);
        if (!lyrics->lines)
                return 0;

        char lineBuffer[1024];

        while (fgets(lineBuffer, sizeof(lineBuffer), file))
        {
                if (lineBuffer[0] != '[' || !isdigit((unsigned char)lineBuffer[1]))
                        continue;

                int min = 0, sec = 0, cs = 0;
                char text[512] = {0};

                if (sscanf(lineBuffer, "[%d:%d.%d]%511[^\r\n]", &min, &sec, &cs, text) == 4)
                {
                        if (lyrics->count == capacity)
                        {
                                capacity *= 2;
                                LyricsLine *newLines = (LyricsLine *)realloc(lyrics->lines, sizeof(LyricsLine) * capacity);
                                if (!newLines)
                                        return 0;
                                lyrics->lines = newLines;
                        }

                        char *start = text;
                        while (isspace((unsigned char)*start))
                                start++;
                        char *end = start + strlen(start);
                        while (end > start && isspace((unsigned char)*(end - 1)))
                                *(--end) = '\0';

                        lyrics->lines[lyrics->count].timestamp = min * 60.0 + sec + cs / 100.0;
                        lyrics->lines[lyrics->count].text = strdup(start);
                        if (!lyrics->lines[lyrics->count].text)
                                return 0;

                        lyrics->count++;
                }
        }

        lyrics->isTimed = 1;
        return 1;
}

// Source - https://stackoverflow.com/a
// Posted by jmucchiello, modified by community. See post 'Timeline' for change history
// Retrieved 2025-12-31, License - CC BY-SA 4.0

// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

Lyrics* getSyncedLyricsFromLIBLRC(SongData* songMetadata) {
    char* response;
    char* request = buildAPIRequest(*songMetadata);
    if (request == NULL) return NULL;

    response = curlRequest(request);
    if (response == NULL) return NULL;
    free(request);

    char* syncedLyrics = jsonParser(response, "syncedLyrics");
    free(response);

    char* fixedLyrics = str_replace(syncedLyrics, "\\n", "\n");
    free(syncedLyrics);

    FILE* lyricsAsFile = fmemopen(fixedLyrics, strlen(fixedLyrics), "r");

    songMetadata->lyrics = (Lyrics *)calloc(1, sizeof(Lyrics));
    loadTimedLyrics(lyricsAsFile, songMetadata->lyrics);
    fclose(lyricsAsFile);

    return songMetadata->lyrics;
}

int main(int argc, char **argv) {
    TagSettings sonnMetadata = {
        .artist = "Linkin Park",
        .title = "Somewhere I Belong",
        .album = "Meteora" 
    };

    SongData song = { .metadata = &sonnMetadata, .duration = 213 };
    Lyrics* lyrics = getSyncedLyricsFromLIBLRC(&song);
    if (lyrics != NULL)
        printf(lyrics->lines[1].text);
    free(lyrics);

    // printf("%sjohn", fixNewlines("HIII\\nHIII\\nHIII\\n"));
    // printf("%s\n", lyrics);
    // free(lyrics);

    // printf("%d\n", LRCExists("john.lrc"));

    return 0;
}
