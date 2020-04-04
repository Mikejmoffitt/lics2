#ifndef COMMON_H
#define COMMON_H

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

static inline void memset(void *s, int c, int n)
{
	uint8_t *d = (uint8_t *)s;
	while (n--) *d++ = c;
}

#endif  // COMMON_H
