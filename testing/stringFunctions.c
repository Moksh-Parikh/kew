#include "stringFunctions.h"

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

// Source - https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
// Posted by jmucchiello, modified by community. See post 'Timeline' for change history
// Retrieved 2025-12-31, License - CC BY-SA 4.0
// Modified by me
char *str_replace(char *originalString, char *subStringToReplace, char *replacementString) {
    char *result;
    char *insertPointer;
    char *tmp;
    int len_subStringToReplace;
    int len_replacementString;
    int prevMatchCurrMatchGap;
    int numberOfReplacements;

    if (!originalString || !subStringToReplace)
        return NULL;

    len_subStringToReplace = strlen(subStringToReplace);
    if (len_subStringToReplace == 0)
        return NULL; // empty subStringToReplace causes infinite loop during counting
    if (!replacementString)
        replacementString = "";
    len_replacementString = strlen(replacementString);

    insertPointer = originalString;
    for (numberOfReplacements = 0; (tmp = strstr(insertPointer, subStringToReplace)); ++numberOfReplacements) {
        insertPointer = tmp + len_subStringToReplace;
    }

    result = calloc(
                strlen(originalString) +
                (len_replacementString - len_subStringToReplace) *
                numberOfReplacements +
                1,
                sizeof(char)
            );

    if (!result)
        return NULL;

    tmp = result;

    while (numberOfReplacements--) {
        insertPointer = strstr(originalString, subStringToReplace);
        prevMatchCurrMatchGap = insertPointer - originalString;
        tmp = strncpy(tmp, originalString, prevMatchCurrMatchGap) + prevMatchCurrMatchGap;
        tmp = strncpy(tmp, replacementString, len_replacementString) + len_replacementString;
        
        originalString += prevMatchCurrMatchGap + len_subStringToReplace;
    }
    strncpy(tmp, originalString, strlen(originalString) - 1);
    // printf("tmp: %s, result: %s\n", tmp, result);

    return result;
}
