void strtolower(char *s);
int hasinput(int fd);
int writen(int fd, void *data, int n);
int readn(int fd, void *data, int n);
int dial(const char *host, int port);
void serve(int port, void (*handlecon)(int, void*), void *arg);
void nodelay(int fd);

void *createseg(const char *name, size_t sz);
void *attachseg(const char *name, size_t sz);
