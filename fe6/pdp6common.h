typedef uint64_t word;
typedef uint32_t hword;

word fw(hword l, hword r);
word point(word pos, word sz, hword p);
hword left(word w);
hword right(word w);
word negw(word w);
int isneg(word w);

void writew(word w, FILE *fp);
word readw(FILE *fp);
void writewbak(word w, FILE *fp);
word readwbak(FILE *fp);
void writewits(word w, FILE *fp);
word readwits(FILE *fp);

void decompdbl(double d, int *s, word *e, uint64_t *m);
word dtopdp(double d);
double pdptod(word f);

char ascii2rad(char c);
char rad2ascii(char c);
int israd50(char c);
word rad50(int n, const char *s);
int unrad50(word r, char *s);
char ascii2sixbit(char c);
char sixbit2ascii(char c);
int issixbit(char c);
word sixbit(const char *s);
void unsixbit(word sx, char *s);
char *disasm(word w);

extern char *mnemonics[448];
extern char *iomnemonics[8];
