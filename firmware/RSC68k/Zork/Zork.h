#ifndef _ZORK_H_
#define _ZORK_H_

extern FILE *Zorkfopen(const char *path, const char *pefilemode);
extern int Zorkfread(void *pvBuffer, size_t size, size_t nmemb, FILE *psFile);
extern int Zorkgetc(FILE *psFile);
extern int Zorkfclose(FILE *psFile);
extern int Zorkfseek(FILE *psFile, long offset, int whence);
extern long Zorkftell(FILE *psFile);
extern char *Zorkfgets(char *str, int size, FILE *psFile);
extern const uint8_t g_u8dtextc[] __attribute__ ((aligned (8)));
extern const uint32_t g_u32dtextcSize;
extern void Zorkmain(int argc, char **argv);

#endif

