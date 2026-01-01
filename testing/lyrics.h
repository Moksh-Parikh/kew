#ifndef LYRICS_H
#define LYRICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <unistd.h>
#include <ctype.h>
#include <limits.h>

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

bool LRCExists(char* originalFilePath);
static int loadTimedLyrics(FILE *file, Lyrics *lyrics);
Lyrics* getSyncedLyricsFromLIBLRC(SongData* songMetadata);
void freeLyrics(Lyrics *lyrics);

#endif
