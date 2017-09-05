#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>

#define STRUCT_MEMBER_POS(t,m)      ((unsigned long)(&(((t *)(0))->m)))
#define STRUCT_MEMBER_SIZE(t,m)     sizeof(((t *)0)->m)

#define DBG(fmt, ...)  fprintf(stdout, "%s[%d] " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__)
#define ERR(fmt, ...)  fprintf(stderr, "%s[%d] ERR " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__)

void dumpbytes(uint8_t *buff, size_t len);

#endif // UTILS_H
