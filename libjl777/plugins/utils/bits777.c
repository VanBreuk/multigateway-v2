//
//  bits777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_bits777_h
#define crypto777_bits777_h
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "system777.c"

#define SMALLVAL 0.000000000000001
#define SATOSHIDEN 100000000L

#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

#define NUM_BLOOMPRIMES 8

struct bloombits { uint8_t hashbits[NUM_BLOOMPRIMES][79997/8 + 1],pad[sizeof(uint64_t)]; };
union _bits256 { uint8_t bytes[32]; uint16_t ushorts[16]; uint32_t uints[8]; uint64_t ulongs[4]; uint64_t txid; };
typedef union _bits256 bits256;
union _bits384 { bits256 sig; uint8_t bytes[48]; uint16_t ushorts[24]; uint32_t uints[12]; uint64_t ulongs[6]; uint64_t txid; };
typedef union _bits384 bits384;

int32_t bitweight(uint64_t x);
int wt384(bits384 a,bits384 b);
int spectrum384(int *spectrum,bits384 a,bits384 b);
uint64_t calc_nxt64bits(const char *str);
int32_t expand_nxt64bits(char *str,uint64_t nxt64bits);
char *nxt64str(uint64_t nxt64bits);
char *nxt64str2(uint64_t nxt64bits);

void set_bloombits(struct bloombits *bloom,uint64_t nxt64bits);
int32_t in_bloombits(struct bloombits *bloom,uint64_t nxt64bits);
void merge_bloombits(struct bloombits *_dest,struct bloombits *_src);
double cos64bits(uint64_t x,uint64_t y);
uint32_t _crc32(uint32_t crc,const void *buf,size_t size);

void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],unsigned char hash[256 >> 3],unsigned char *src,int32_t len);
void calc_sha256cat(unsigned char hash[256 >> 3],unsigned char *src,int32_t len,unsigned char *src2,int32_t len2);

#endif
#else
#ifndef crypto777_bits777_c
#define crypto777_bits777_c

#ifndef crypto777_bits777_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif
static uint64_t bloomprimes[NUM_BLOOMPRIMES] = { 79559, 79631, 79691, 79697, 79811, 79841, 79901, 79997 };

int32_t bitweight(uint64_t x)
{
    int i,wt = 0;
    for (i=0; i<64; i++)
        if ( (1 << i) & x )
            wt++;
    return(wt);
}

int wt384(bits384 a,bits384 b)
{
    int i,n = 0;
    for (i=0; i<6; i++)
        n += bitweight(a.ulongs[i] ^ b.ulongs[i]);
    return(n);
}

int spectrum384(int *spectrum,bits384 a,bits384 b)
{
    int i,n = 0;
    for (i=0; i<384; i++)
        if ( GETBIT(a.bytes,i) ^ GETBIT(b.bytes,i) )
            spectrum[i]++, n++;
    return(n);
}

int32_t expand_nxt64bits(char *NXTaddr,uint64_t nxt64bits)
{
    int32_t i,n;
    uint64_t modval;
    char rev[64];
    for (i=0; nxt64bits!=0; i++)
    {
        modval = nxt64bits % 10;
        rev[i] = (char)(modval + '0');
        nxt64bits /= 10;
    }
    n = i;
    for (i=0; i<n; i++)
        NXTaddr[i] = rev[n-1-i];
    NXTaddr[i] = 0;
    return(n);
}

char *nxt64str(uint64_t nxt64bits)
{
    static char NXTaddr[64];
    expand_nxt64bits(NXTaddr,nxt64bits);
    return(NXTaddr);
}

char *nxt64str2(uint64_t nxt64bits)
{
    static char NXTaddr[64];
    expand_nxt64bits(NXTaddr,nxt64bits);
    return(NXTaddr);
}

int32_t cmp_nxt64bits(const char *str,uint64_t nxt64bits)
{
    char expanded[64];
    if ( str == 0 )//|| str[0] == 0 || nxt64bits == 0 )
        return(-1);
    if ( nxt64bits == 0 && str[0] == 0 )
        return(0);
    expand_nxt64bits(expanded,nxt64bits);
    return(strcmp(str,expanded));
}

uint64_t calc_nxt64bits(const char *NXTaddr)
{
    int32_t c;
    int64_t n,i;
    uint64_t lastval,mult,nxt64bits = 0;
    if ( NXTaddr == 0 )
    {
        printf("calling calc_nxt64bits with null ptr!\n");
        return(0);
    }
    n = strlen(NXTaddr);
    if ( n >= 22 )
    {
        printf("calc_nxt64bits: illegal NXTaddr.(%s) too long\n",NXTaddr);
        return(0);
    }
    else if ( strcmp(NXTaddr,"0") == 0 || strcmp(NXTaddr,"false") == 0 )
    {
        // printf("zero address?\n"); getchar();
        return(0);
    }
    mult = 1;
    lastval = 0;
    for (i=n-1; i>=0; i--,mult*=10)
    {
        c = NXTaddr[i];
        if ( c < '0' || c > '9' )
        {
            printf("calc_nxt64bits: illegal char.(%c %d) in (%s).%d\n",c,c,NXTaddr,(int32_t)i);
#ifdef __APPLE__
            while ( 1 )
            {
                portable_sleep(60);
                printf("calc_nxt64bits: illegal char.(%c %d) in (%s).%d\n",c,c,NXTaddr,(int32_t)i);
            }
#endif
            return(0);
        }
        nxt64bits += mult * (c - '0');
        if ( nxt64bits < lastval )
            printf("calc_nxt64bits: warning: 64bit overflow %llx < %llx\n",(long long)nxt64bits,(long long)lastval);
        lastval = nxt64bits;
    }
    if ( cmp_nxt64bits(NXTaddr,nxt64bits) != 0 )
        printf("error calculating nxt64bits: %s -> %llx -> %s\n",NXTaddr,(long long)nxt64bits,nxt64str(nxt64bits));
    return(nxt64bits);
}

void set_bloombits(struct bloombits *bloom,uint64_t nxt64bits)
{
    int32_t i;
    for (i=0; i<NUM_BLOOMPRIMES; i++)
        SETBIT(bloom->hashbits[i],(nxt64bits % bloomprimes[i]));
}

int32_t in_bloombits(struct bloombits *bloom,uint64_t nxt64bits)
{
    int32_t i;
    for (i=0; i<NUM_BLOOMPRIMES; i++)
    {
        if ( GETBIT(bloom->hashbits[i],(nxt64bits % bloomprimes[i])) == 0 )
        {
            //printf("%llu not in bloombits.%llu\n",(long long)nxt64bits,(long long)ref64bits);
            return(0);
        }
    }
    //printf("%llu in bloombits.%llu\n",(long long)nxt64bits,(long long)ref64bits);
    return(1);
}

void merge_bloombits(struct bloombits *_dest,struct bloombits *_src)
{
    int32_t i,n = (int32_t)(sizeof(*_dest) / sizeof(uint64_t));
    uint64_t *dest,*src;
    dest = (uint64_t *)&_dest->hashbits[0][0];
    src = (uint64_t *)&_src->hashbits[0][0];
    for (i=0; i<n; i++)
        dest[i] |= src[i];
}

double cos64bits(uint64_t x,uint64_t y)
{
    static double sqrts[65],sqrtsB[64*64+1];
    int32_t i,wta,wtb,dot;
    if ( sqrts[1] == 0. )
    {
        for (i=1; i<=64; i++)
            sqrts[i] = sqrt(i);
        for (i=1; i<=64*64; i++)
            sqrtsB[i] = sqrt(i);
    }
    for (i=wta=wtb=dot=0; i<64; i++,x>>=1,y>>=1)
    {
        if ( (x & 1) != 0 )
        {
            wta++;
            if ( (y & 1) != 0 )
                wtb++, dot++;
        }
        else if ( (y & 1) != 0 )
            wtb++;
    }
    if ( wta != 0 && wtb != 0 )
    {
        static double errsum,errcount;
        double cosA,cosB;
        errcount += 1.;
        cosA = ((double)dot / (sqrts[wta] * sqrts[wtb]));
        cosB = ((double)dot / (sqrtsB[wta * wtb]));
        errsum += fabs(cosA - cosB);
        if ( fabs(cosA - cosB) > SMALLVAL )
            printf("cosA %f vs %f [%.20f] ave error [%.20f]\n",cosA,cosB,cosA-cosB,errsum/errcount);
        return(cosB);
    }
    return(0.);
}

static const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t _crc32(uint32_t crc,const void *buf,size_t size)
{
	const uint8_t *p;
    
	p = (const uint8_t *)buf;
	crc = crc ^ ~0U;
    
	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    
	return crc ^ ~0U;
}


#endif
#endif