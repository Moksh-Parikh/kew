#include "stringFunctions.h"
#include "network.h"
#include "lyrics.h"

int main(int argc, char **argv) {
    TagSettings sonnMetadata = {
        .artist = "Linkin Park",
        .title = "Somewhere I Belong",
        .album = "Meteora" 
    };

    SongData song = { .metadata = &sonnMetadata, .duration = 213 };
    Lyrics* lyrics = getSyncedLyricsFromLIBLRC(&song);
    if (lyrics != NULL && lyrics->lines != NULL &&
        lyrics->lines[1].text != NULL
    )
        printf(lyrics->lines[1].text);

    freeLyrics(lyrics);

    return 0;
}
