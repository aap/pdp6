#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

//typedef uint64_t u64;
typedef unsigned long long u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
#define nil NULL

#define nelem(array) (sizeof(array)/sizeof(array[0]))

void panic(const char *fmt, ...);
int hasinput(int fd);
int dial(const char *host, int port);
int serve1(int port);
void nodelay(int fd);

void *createseg(const char *name, size_t sz);
void *attachseg(const char *name, size_t sz);

void inittime(void);
u64 gettime(void);
void nsleep(u64 ns);
#define NEVER (~0)

char **split(char *line, int *pargc);


typedef struct FD FD;
struct FD
{
	int fd;
	int ready;
	int id;
};
void startpolling(void);
void waitfd(FD *fd);
void closefd(FD *fd);
