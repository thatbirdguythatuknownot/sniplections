#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

typedef long long i64;
typedef unsigned int u32;
typedef unsigned char u8;

#define MD5_BLOCKSIZE    64
#define MD5_DIGESTSIZE   16

#define ROLc(x, y) ((((u32)(x)<<(u32)((y)&31)) | (((u32)(x)&0xFFFFFFFFUL)>>(u32)(32-((y)&31)))) & 0xFFFFFFFFUL)

#define LOAD32(i, x)                \
    ((u32)((x)[i + 3] & 255)<<24) | \
    ((u32)((x)[i + 2] & 255)<<16) | \
    ((u32)((x)[i + 1] & 255)<<8)  | \
    ((u32)((x)[i] & 255))

#define STORE64(i, x, y) memcpy((y) + i, &x, 8)

#define STORE32(i, x, y) memcpy((y) + i, &x, 4)

#define F(x,y,z)  (z ^ (x & (y ^ z)))
#define G(x,y,z)  (y ^ (z & (y ^ x)))
#define H(x,y,z)  (x^y^z)
#define I(x,y,z)  (y^(x|(~z)))

#define FF(a,b,c,d,M,s,t) \
    a = (a + F(b,c,d) + M + t); a = ROLc(a, s) + b;

#define GG(a,b,c,d,M,s,t) \
    a = (a + G(b,c,d) + M + t); a = ROLc(a, s) + b;

#define HH(a,b,c,d,M,s,t) \
    a = (a + H(b,c,d) + M + t); a = ROLc(a, s) + b;

#define II(a,b,c,d,M,s,t) \
    a = (a + I(b,c,d) + M + t); a = ROLc(a, s) + b;

#define COMPRESS(buf) \
    u32  W0 = LOAD32(0,  buf),  W1 = LOAD32(4,  buf),  W2 = LOAD32(8,  buf),  W3 = LOAD32(12, buf), \
         W4 = LOAD32(16, buf),  W5 = LOAD32(20, buf),  W6 = LOAD32(24, buf),  W7 = LOAD32(28, buf), \
         W8 = LOAD32(32, buf),  W9 = LOAD32(36, buf), W10 = LOAD32(40, buf), W11 = LOAD32(44, buf), \
        W12 = LOAD32(48, buf), W13 = LOAD32(52, buf), W14 = LOAD32(56, buf), W15 = LOAD32(60, buf); \
    u32 a = A, b = B, c = C, d = D; \
    FF(a,b,c,d,W0,7,0xd76aa478UL) \
    FF(d,a,b,c,W1,12,0xe8c7b756UL) \
    FF(c,d,a,b,W2,17,0x242070dbUL) \
    FF(b,c,d,a,W3,22,0xc1bdceeeUL) \
    FF(a,b,c,d,W4,7,0xf57c0fafUL) \
    FF(d,a,b,c,W5,12,0x4787c62aUL) \
    FF(c,d,a,b,W6,17,0xa8304613UL) \
    FF(b,c,d,a,W7,22,0xfd469501UL) \
    FF(a,b,c,d,W8,7,0x698098d8UL) \
    FF(d,a,b,c,W9,12,0x8b44f7afUL) \
    FF(c,d,a,b,W10,17,0xffff5bb1UL) \
    FF(b,c,d,a,W11,22,0x895cd7beUL) \
    FF(a,b,c,d,W12,7,0x6b901122UL) \
    FF(d,a,b,c,W13,12,0xfd987193UL) \
    FF(c,d,a,b,W14,17,0xa679438eUL) \
    FF(b,c,d,a,W15,22,0x49b40821UL) \
    GG(a,b,c,d,W1,5,0xf61e2562UL) \
    GG(d,a,b,c,W6,9,0xc040b340UL) \
    GG(c,d,a,b,W11,14,0x265e5a51UL) \
    GG(b,c,d,a,W0,20,0xe9b6c7aaUL) \
    GG(a,b,c,d,W5,5,0xd62f105dUL) \
    GG(d,a,b,c,W10,9,0x02441453UL) \
    GG(c,d,a,b,W15,14,0xd8a1e681UL) \
    GG(b,c,d,a,W4,20,0xe7d3fbc8UL) \
    GG(a,b,c,d,W9,5,0x21e1cde6UL) \
    GG(d,a,b,c,W14,9,0xc33707d6UL) \
    GG(c,d,a,b,W3,14,0xf4d50d87UL) \
    GG(b,c,d,a,W8,20,0x455a14edUL) \
    GG(a,b,c,d,W13,5,0xa9e3e905UL) \
    GG(d,a,b,c,W2,9,0xfcefa3f8UL) \
    GG(c,d,a,b,W7,14,0x676f02d9UL) \
    GG(b,c,d,a,W12,20,0x8d2a4c8aUL) \
    HH(a,b,c,d,W5,4,0xfffa3942UL) \
    HH(d,a,b,c,W8,11,0x8771f681UL) \
    HH(c,d,a,b,W11,16,0x6d9d6122UL) \
    HH(b,c,d,a,W14,23,0xfde5380cUL) \
    HH(a,b,c,d,W1,4,0xa4beea44UL) \
    HH(d,a,b,c,W4,11,0x4bdecfa9UL) \
    HH(c,d,a,b,W7,16,0xf6bb4b60UL) \
    HH(b,c,d,a,W10,23,0xbebfbc70UL) \
    HH(a,b,c,d,W13,4,0x289b7ec6UL) \
    HH(d,a,b,c,W0,11,0xeaa127faUL) \
    HH(c,d,a,b,W3,16,0xd4ef3085UL) \
    HH(b,c,d,a,W6,23,0x04881d05UL) \
    HH(a,b,c,d,W9,4,0xd9d4d039UL) \
    HH(d,a,b,c,W12,11,0xe6db99e5UL) \
    HH(c,d,a,b,W15,16,0x1fa27cf8UL) \
    HH(b,c,d,a,W2,23,0xc4ac5665UL) \
    II(a,b,c,d,W0,6,0xf4292244UL) \
    II(d,a,b,c,W7,10,0x432aff97UL) \
    II(c,d,a,b,W14,15,0xab9423a7UL) \
    II(b,c,d,a,W5,21,0xfc93a039UL) \
    II(a,b,c,d,W12,6,0x655b59c3UL) \
    II(d,a,b,c,W3,10,0x8f0ccc92UL) \
    II(c,d,a,b,W10,15,0xffeff47dUL) \
    II(b,c,d,a,W1,21,0x85845dd1UL) \
    II(a,b,c,d,W8,6,0x6fa87e4fUL) \
    II(d,a,b,c,W15,10,0xfe2ce6e0UL) \
    II(c,d,a,b,W6,15,0xa3014314UL) \
    II(b,c,d,a,W13,21,0x4e0811a1UL) \
    II(a,b,c,d,W4,6,0xf7537e82UL) \
    II(d,a,b,c,W11,10,0xbd3af235UL) \
    II(c,d,a,b,W2,15,0x2ad7d2bbUL) \
    II(b,c,d,a,W9,21,0xeb86d391UL) \
    A += a; \
    B += b; \
    C += c; \
    D += d;

#define HASH_ALGO(string, var) \
    { \
        u32 curlen = 0; \
        i64 size_string, n, length = 0; \
        u8 *buffer = _buffer; \
        size_string = snprintf((char *)buffer, sizeof(string) + 7, string "%d", var); \
        u8 md5_buffer[MD5_BLOCKSIZE]; \
        while (likely(size_string > 0)) { \
            if (curlen == 0 && size_string >= MD5_BLOCKSIZE) { \
                COMPRESS(buffer); \
                length += MD5_BLOCKSIZE * 8; \
                buffer += MD5_BLOCKSIZE; \
                size_string += MD5_BLOCKSIZE; \
            } \
            else { \
                n = (i64)(MD5_BLOCKSIZE - curlen); \
                if (size_string < n) { \
                    n = size_string; \
                } \
                memcpy(md5_buffer + curlen, buffer, (size_t)n); \
                curlen += (u32)n; \
                buffer += n; \
                size_string -= n; \
                if (curlen == MD5_BLOCKSIZE) { \
                    COMPRESS(md5_buffer); \
                    length += MD5_BLOCKSIZE * 8; \
                    curlen = 0; \
                } \
            } \
        } \
        u8 out[MD5_DIGESTSIZE]; \
        int i; \
        length += curlen * 8; \
        md5_buffer[curlen++] = (u8)0x80; \
        if (unlikely(curlen > 56)) { \
            memset(md5_buffer + curlen, 0, 64 - curlen); \
            COMPRESS(md5_buffer); \
            curlen = 0; \
        } \
        memset(md5_buffer + curlen, 0, 56 - curlen); \
        STORE64(56, length, md5_buffer); \
        COMPRESS(md5_buffer); \
    }

#define STRING "iwrupvqb"

#if 0
#define omp_get_num_procs() 1
#endif

int main() {
    u8 *_buffer = (u8 *)calloc(sizeof(STRING) + 7, 1);
    u32 A, B, C, D;
    int p2 = 0, p1 = 0, not_done = 1;
    int chunk_size = (int)ceil(1000000. * (10. / omp_get_num_procs()));
    i64 size_num = 1;
    LARGE_INTEGER start_time, end_time, frequency;
    QueryPerformanceCounter(&start_time);
    do {
        A = 0x67452301UL, B = 0xefcdab89UL, C = 0x98badcfeUL, D = 0x10325476UL;
        p1++;
        HASH_ALGO(STRING, p1)
    } while (likely((A & 0xf0ffff) != 0));
#pragma omp parallel private(A, B, C, D) num_threads(omp_get_num_procs())
    {
        int _p2 = omp_get_thread_num() * chunk_size;
        int compare_to = _p2 + chunk_size;
        for (_p2 += 1; likely(not_done && _p2 <= compare_to); ++_p2) {
            A = 0x67452301UL, B = 0xefcdab89UL, C = 0x98badcfeUL, D = 0x10325476UL;
            HASH_ALGO(STRING, _p2)
            if (unlikely((A & 0xffffff) == 0)) {
                not_done = 0;
                p2 = _p2;
            }
        }
    }
    QueryPerformanceCounter(&end_time);
    QueryPerformanceFrequency(&frequency);
    double elapsed = (double)(end_time.QuadPart - start_time.QuadPart) * 1000.0 / (double)frequency.QuadPart;
    printf("input: %s\np1: %d, p2: %d\nelapsed: %g ms\nnumthreads: %d\nelapsed / numthreads: %g ms\n", STRING, p1, p2, elapsed, omp_get_num_procs(), elapsed/omp_get_num_procs());
    return 0;
}
