#include "img-panda/common.h"

int memsearch(const char *hay, int haysize, const char *needle, int needlesize) {
    int haypos, needlepos;
    haysize -= needlesize;
    for (haypos = 0; haypos <= haysize; haypos++) {
        for (needlepos = 0; needlepos < needlesize; needlepos++) {
            if (hay[haypos + needlepos] != needle[needlepos]) {
                // Next character in haystack.
                break;
            }
        }
        if (needlepos == needlesize) {
            return haypos;
        }
    }
    return -1;
};