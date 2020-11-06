/*
 *	kprint.c
 *
 *	Functions for printing message.
 *
 *	Written by:     K.J. Lin <kjlin@faraday-tech.com>
 *	
 *		Copyright (c) 2009 FARADAY, ALL Rights Reserve
 */
#include "kprint.h"

extern void serial_putc(unsigned char);
extern int strlen(const char *);

void putc(const char c)
{
	serial_putc(c);
	if ( c == '\n' )
		serial_putc('\r');
}

void puts(const char *s)
{
	char c;
	while ( ( c = *s++ ) != '\0' ) {
		serial_putc(c);
		if ( c == '\n' ) serial_putc('\r');
	}
}

void puthex(unsigned long val)
{
	unsigned char buf[10];
	int i;
	for (i = 7;  i >= 0;  i--)
	{
		buf[i] = "0123456789ABCDEF"[val & 0x0F];
		val >>= 4;
	}
	buf[8] = '\0';
//	puts(buf);	TBD
}

#define FALSE 0
#define TRUE  1
#define is_digit(c) ((c >= '0') && (c <= '9'))

int _cvt(unsigned long val, char *buf, long radix, char *digits)
{
        char temp[80];
        char *cp = temp;
        int length = 0;
        if (val == 0)
        { /* Special case */
                *cp++ = '0';
        } else
                while (val)
                {
                        *cp++ = digits[val % radix];
                        val /= radix;
                }
        while (cp != temp)
        {
                *buf++ = *--cp;
                length++;
        }
        *buf = '\0';
        return (length);
}

void raise(void)
{
  
}
void _vprintk(void(*putc)(const char), const char *fmt0, va_list ap)
{
        char c, sign, *cp = 0;
        int left_prec, right_prec, zero_fill, length = 0, pad, pad_on_right;
        char buf[32];
        long val;
        while ((c = *fmt0++))
        {
                if (c == '%')
                {
                        c = *fmt0++;
                        left_prec = right_prec = pad_on_right = 0;
                        if (c == '-')
                        {
                                c = *fmt0++;
                                pad_on_right++;
                        }
                        if (c == '0')
                        {
                                zero_fill = TRUE;
                                c = *fmt0++;
                        } else
                        {
                                zero_fill = FALSE;
                        }
                        while (is_digit(c))
                        {
                                left_prec = (left_prec * 10) + (c - '0');
                                c = *fmt0++;
                        }
                        if (c == '.')
                        {
                                c = *fmt0++;
                                zero_fill++;
                                while (is_digit(c))
                                {
                                        right_prec = (right_prec * 10) + (c - '0');
                                        c = *fmt0++;
                                }
                        } else
                        {
                                right_prec = left_prec;
                        }
                        sign = '\0';
                        switch (c)
                        {
                        case 'd':
                        case 'x':
                        case 'X':
                                val = va_arg(ap, long);
                                switch (c)
                                {
                                case 'd':
                                        if (val < 0)
                                        {
                                                sign = '-';
                                                val = -val;
                                        }
                                        length = _cvt(val, buf, 10, "0123456789");
                                        break;
                                case 'x':
                                        length = _cvt(val, buf, 16, "0123456789abcdef");
                                        break;
                                case 'X':
                                        length = _cvt(val, buf, 16, "0123456789ABCDEF");
                                        break;
                                }
                                cp = buf;
                                break;
                        case 's':
                                cp = va_arg(ap, char *);
                                length = strlen(cp);
                                break;
                        case 'c':
                                c = va_arg(ap, long /*char*/);
                                (*putc)(c);
                                continue;
                        default:
                                (*putc)('?');
                        }
                        pad = left_prec - length;
                        if (sign != '\0')
                        {
                                pad--;
                        }
                        if (zero_fill)
                        {
                                c = '0';
                                if (sign != '\0')
                                {
                                        (*putc)(sign);
                                        sign = '\0';
                                }
                        } else
                        {
                                c = ' ';
                        }
                        if (!pad_on_right)
                        {
                                while (pad-- > 0)
                                {
                                        (*putc)(c);
                                }
                        }
                        if (sign != '\0')
                        {
                                (*putc)(sign);
                        }
                        while (length-- > 0)
                        {
                                (*putc)(c = *cp++);
                                if (c == '\n')
                                {
                                        (*putc)('\r');
                                }
                        }
                        if (pad_on_right)
                        {
                                while (pad-- > 0)
                                {
                                        (*putc)(c);
                                }
                        }
                } else
                {
                        (*putc)(c);
                        if (c == '\n')
                        {
                                (*putc)('\r');
                        }
                }
        }
}

void KPrint(char const *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        _vprintk(putc, fmt, ap);
        va_end(ap);
        return;
}
