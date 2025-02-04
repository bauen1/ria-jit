
#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/time.h>
#include <linux/resource.h>
#include <linux/unistd.h>
#include <linux/stat.h>

#define ssize_t intptr_t
#define off_t intptr_t

#define __timeval_defined
#define __timespec_defined

extern char **environ;
extern char **__environ;

//TODO This is a very temporary solution since the actual startup routine of minilib is not yet used.
#ifndef NO_STDLIB
extern const size_t *auxvptr_temp;
#endif

long syscall(long, long, long, long, long, long, long);

__attribute__((noreturn)) void _exit(int status);

int getpid(void);

int clock_gettime(int clk_id, struct timespec *tp);

int open(const char *pathname, int flags, int mode);

int openat(int dirfd, const char *pathname, int flags, int mode);

off_t lseek(int fd, off_t offset, int whence);

ssize_t read(int fd, void *buf, size_t count);

ssize_t write(int fd, const void *buf, size_t count);

int close(int fd);

int fcntl(int fd);

int fstatx(int fd, struct statx *buf);

ssize_t read_full(int fd, void *buf, size_t nbytes);

ssize_t write_full(int fd, const void *buf, size_t nbytes);

//sys/resource.h
int getrlimit(int resource, struct rlimit *rlimits);

// sys/auxv.h
unsigned long int getauxval(unsigned long int __type);

// sys/mman.h
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

int munmap(void *addr, size_t length);

int mprotect(void *addr, size_t len, int prot);

// stdio.h
int vsnprintf(char *str, size_t size, const char *restrict format, va_list args);

int snprintf(char *str, size_t size, const char *restrict format, ...);

int vdprintf(int fd, const char *restrict format, va_list args);

__attribute__((__format__(printf, 2, 0)))

int dprintf(int fd, const char *restrict format, ...)
__attribute__((__format__(printf, 2, 3)));

int printf(const char *restrict format, ...);

int puts(const char *s);

// strings.h
size_t strlen(const char *s);

int strcmp(const char *s1, const char *s2);

int strncmp(const char *s1, const char *s2, size_t n);

char *strchr(const char *s, int c);

void *memset(void *s, int c, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

void *memcpy(void *dest, const void *src, size_t n);

int execve(const char *filename, char *const argv[], char *const envp[]);

int dup2(int oldfd, int newfd);

int pipe2(int pipefd[2], int flags);

int __clone(int (*func)(void *), void *stack, int flags, void *arg, ...);

#define STRINGIFY_ARG(x) #x
#define STRINGIFY(x) STRINGIFY_ARG(x)

#define PASTE_ARGS(a, b) a ## b
#define PASTE(a, b) PASTE_ARGS(a, b)

#define ALIGN_DOWN(v, a) ((v) & ~((a)-1))
#define ALIGN_UP(v, a) (((v) + ((a) - 1)) & ~((a)-1))

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

#define ASM_BLOCK(...) __asm__(#__VA_ARGS__)

#if __SIZEOF_POINTER__ == 8
#define BAD_ADDR(a) (((uintptr_t) (a)) > 0xfffffffffffff000ULL)
#else
#define BAD_ADDR(a) (((uintptr_t) (a)) > 0xfffff000UL)
#endif

#endif
