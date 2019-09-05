#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

struct FileStream
{
	FILE *f;
};

struct FileStream *fs_init(struct FileStream *_this, const char *_fileName);
void fs_finalize(struct FileStream *_this);
struct FileStream *fs_print(struct FileStream *_this, const char *pszFmt, ...);

#endif