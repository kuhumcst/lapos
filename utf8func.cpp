/*
LETTERFUNC - Unicode functions: I/O, string comparison, change casing

Copyright (C) 2012  Center for Sprogteknologi, University of Copenhagen

This file is part of CST's Language Technology Tools.

LETTERFUNC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

LETTERFUNC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LETTERFUNC; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "utf8func.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#if UNICODE_CAPABLE
#include "unicaseconv.h"

bool globUTF8 = true; /* can be set to false by calls to 
                  isUpperUTF8, 
                  AllToUpper, 
                  allToLowerUTF8, 
                  NToLowerUTF8, 
                  AllToLowerUTF8, 
                  Utf8ToUnicode
                  */
bool Turcic = false;

unsigned int upperEquivalent(int kar)
    {
    if(kar == 'i' && !Turcic)
        return 'I';
    return toUpperUnicode(kar);
    }

unsigned int lowerEquivalent(int kar)
    {
    if(kar == 'I' && !Turcic)
        return 'i';
    return toLowerUnicode(kar);
    }

bool isUpperUTF8(const char* s)
    {
    int S = UTF8char(s, globUTF8);
    if(!globUTF8)
        return false;
    return (upperEquivalent(S) != lowerEquivalent(S)) //2026.11.02
        && (upperEquivalent(S) == (unsigned int)S);
    }

int UnicodeToUtf8(unsigned int w,char * s,size_t len)
// Converts Unicode character w to 1,2,3 or 4 bytes of UTF-8 in s.
// Returns number of bytes written.
    {
    // Convert unicode to utf8
    int bytes;
    if ( w < 0x0080 )
        {
        if(len < 1)
            return 0;
        *s = (char)w;
        bytes = 1;
        }
    else 
        {
        if(w < 0x0800) // 7FF = 1 1111 111111
            {
            if(len < 2)
                return 0;
            *s++ = (char)(0xc0 | (w >> 6 ));
            bytes = 2;
            }
        else
            {
            if(w < 0x10000) // FFFF = 1111 111111 111111
                {
                if(len < 3)
                    return 0;
                *s++ = (char)(0xe0 | (w >> 12));
                bytes = 3;
                } 
            else // w < 110000
                { // 10000 = 010000 000000 000000, 10ffff = 100 001111 111111 111111
                if(len < 4)
                    return 0;
                *s++ = (char)(0xf0 | (w >> 18));
                *s++ = (char)(0x80 | ((w >> 12) & 0x3f));
                bytes = 4;
                } 
            *s++ = (char)(0x80 | ((w >> 6) & 0x3f));
            }
        *s = (char)(0x80 | (w & 0x3f));
        }
    return bytes;
    }


static int utf8bytes(unsigned long val)
    {
    if(val < 0x80)
        {
        return 1;
        }
    else if(val < 0x0800) // 7FF = 1 1111 111111
        {
        return 2;
        }
    else if(val < 0x10000) // FFFF = 1111 111111 111111
        {
        return 3;
        } 
    else if(val < 0x200000) 
        { // 10000 = 010000 000000 000000, 10ffff = 100 001111 111111 111111
        return 4;
        } 
    else if(val < 0x4000000)
        {
        return 5;
        }
    else
        {
        return 6;
        }
    }



const char * changeCase(const char * s,bool low,size_t & length)
// The returned value must be copied to its destination before the next call
// to changeCase. 'length' is the number of bytes before the final '\0' in the
// returned value. On entry, if length is set to a positive number, case is
// only converted for the first 'length' bytes; the remaining bytes are
// skipped.
    {
    size_t len;
    len = strlen(s);
    if(len > 0)
        {
        static char * obuf = NULL;
        len += sizeof(int);
        len /= sizeof(int);
        len *= sizeof(int); // string fits within integer boundaries.
        if(obuf)
            delete [] obuf;
        obuf = new char[len];
        char * dwarn;
        char * d = obuf;
        const char * sstop = ((length == 0 || length > len) ? s+len : s + length);
        dwarn = obuf + len - 7;
        bool isutf = true;
        for(;s < sstop && *s;)
            {
            int S = getUTF8char(s,isutf);
            if(!isutf)
                {
                strcpy(d,s);
                length = (size_t)((d + strlen(d)) - obuf);
                return obuf;
                }
            unsigned int D = low ? lowerEquivalent(S) : upperEquivalent(S);
            if(d >= dwarn)
                {
                int nb = utf8bytes(D);
                if(d + nb >= dwarn+7)
                    {
                    /* overrun */
                    len *= 2;
                    char * buf = new char[len];
                    dwarn = buf + len - 7;
                    memcpy(buf,obuf,(size_t)(d - obuf));
                    d = buf + (d - obuf);
                    delete [] obuf;
                    obuf = buf;
                    }
                }
            d += UnicodeToUtf8(D,d,len - (d-obuf));
            }
        *d = 0;
        length = (size_t)(d - obuf);
        return obuf;
        }
    length = 0;
    return s;
    }

int getUTF8char(const char *& s,bool & UTF_8) 
/* Returns *s if s isn't UTF-8 and increments s with 1.
   Otherwise returns character and shifts s to start of next character.
   Returns 0 if end of string reached. In that case, s becomes invalid
   (pointing past the zero).
*/
    {
    const unsigned char * save = (const unsigned char *)s;
    int k[6];
    int kk;
    if((kk = *(const unsigned char *)s++) != 0)
        {
        k[0] = kk;
        if((k[0] & 0xc0) == 0xc0) // 11bbbbbb
            {
            int i = 1;
            if((k[0] & 0xfe) == 0xfe)
                {
                s = (const char *)save + 1;
                UTF_8 = false;
                return *save;
                }
            // Start of multibyte
            
            for(int k0 = k[0];(k0 << i) & 0x80;++i)
                {
                k[i] = *s++;
                
                if((k[i] & 0xc0) != 0x80) // 10bbbbbb
                    {
                    s = (const char *)save + 1;
                    UTF_8 = false;
                    return *save;
                    }
                }
            int K = ((k[0] << i) & 0xff) << (5 * i - 6);
            int I = --i;
            while(i > 0)
                {
                K |= (k[i] & 0x3f) << ((I - i) * 6);
                --i;
                }
            if(K <= 0x7f)
                {
                s = (const char *)save + 1;
                UTF_8 = false;
                return *save; // overlong UTF-8 sequence
                }
            return K;
            }
        else
            return k[0];
        }
    else
        return 0;
    }


int UTF8char(const char * s,bool & UTF_8) 
/* Returns *s if s isn't UTF-8.
   Otherwise returns UTF-8 character.
   Returns 0 if end of string reached.
   Does not increment s;
*/
    {
    if(!UTF_8)
        return *s;

    const unsigned char * save = (const unsigned char *)s;
    int k[6];
    int kk;
    if((kk = (unsigned char/*20120906*/)*s++) != 0)
        {
        k[0] = kk;
        if((k[0] & 0xc0) == 0xc0) // 11bbbbbb
            {
            int i = 1;
            if((k[0] & 0xfe) == 0xfe)
                {
                UTF_8 = false;
                return *save;
                }
            // Start of multibyte
            
            for(int k0 = k[0];(k0 << i) & 0x80;++i)
                {
                k[i] = *s++;
                
                if((k[i] & 0xc0) != 0x80) // 10bbbbbb
                    {
                    UTF_8 = false;
                    return *save;
                    }
                }
            int K = ((k[0] << i) & 0xff) << (5 * i - 6);
            int I = --i;
            while(i > 0)
                {
                K |= (k[i] & 0x3f) << ((I - i) * 6);
                --i;
                }
            if(K <= 0x7f)
                {
                UTF_8 = false;
                return *save; // overlong UTF-8 sequence
                }
            return K;
            }
        else
            {
            if(k[0] > 0x7f)
                UTF_8 = false; /*20120906*/
            return k[0];
            }
        }
    else
        return 0;
    }


#endif
