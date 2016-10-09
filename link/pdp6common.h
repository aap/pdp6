typedef uint64_t word;
typedef uint32_t hword;

char ascii2rad(char c);
char rad2ascii(char c);
word rad50(int n, const char *s);
int unrad50(word r, char *s);
char ascii2sixbit(char c);
char sixbit2ascii(char c);
word sixbit(const char *s);
void unsixbit(word sx, char *s);
char *disasm(word w);
