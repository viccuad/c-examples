#ifndef DEFS_H
#define DEFS_H 1

#define bytebajo(w) ((w) & 0377)
#define bytealto(w) bytebajo((w) >> 8)

typedef enum {FALSE, TRUE} BOOL;

#endif /* DEFS_H */
