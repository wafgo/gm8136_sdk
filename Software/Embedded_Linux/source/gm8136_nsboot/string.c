/*
 *	string.c
 *
 *	String libraries (to be added...)
 *
 *	Written by:     K.J. Lin <kjlin@faraday-tech.com>
 *
 *		Copyright (c) 2009 FARADAY, ALL Rights Reserve
 */
int strlen(const char * s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc) ;
	return (int)(sc - s);
}

void *memcpy(void *dst, const void *src, unsigned int n)
{
	char *tmp = dst;
	const char *s = src;

	while(n--)
		*tmp++ = *s++;
	return dst;
}

void memset(void *dst, unsigned char val, unsigned int n)
{
	char *tmp = dst;
	
	while(n--)
		*tmp++ = val;
}

unsigned int read_reg(unsigned int reg)
{
    return *((volatile unsigned int *)reg);
}

void write_reg(unsigned int reg, unsigned int value)
{
    *((volatile unsigned int *)reg) = value;
}

void sal_memset(void *pbuf, unsigned char val, int len)
{
    int     i;
    char    *buff = (char *)pbuf;
    
    for (i = 0; i < len; i ++) {
        buff[i] = val;
    }
}

int sal_memcmp(void *sbuf, void *dbuf, int len)
{
    int     i;
    char    *src, *dst;
    
    src = (char *)sbuf;
    dst = (char *)dbuf;
    
    for (i = 0; i < len; i ++) {   
        if (src[i] != dst[i]) {
            return 1;
        }
    }
    return 0;
}
