#ifndef _SYSTEM_H
#define _SYSTEM_H

#define asm __asm__ volatile

/* #define inline __attribute__((always_inline)) __attribute__((unused)) */
#define inline __attribute__((unused)) __inline

#define RESTRICT __restrict
#define FASTCALL __attribute__((fastcall))
#define STDCALL __attribute((stdcall))

typedef signed char S8;
typedef unsigned char U8;
typedef signed short S16;
typedef unsigned short U16;
typedef signed int S32;
typedef unsigned int U32;
typedef signed long long int S64;
typedef unsigned long long int U64;

#ifndef NULL
#define NULL ((void*) 0)
#endif

/* These are compiled to actual x87 FPU instructions such as FCOS. Cool :)
	Need to implement inline ASM macros for other compilers than GCC */
#define sinf	__builtin_sinf
#define cosf	__builtin_cosf
#define atan2f	__builtin_atan2f
#define tanf	__builtin_tanf
#define sqrtf	__builtin_sqrtf
#define sqrt	__builtin_sqrt
#define fmodf	__builtin_fmodf
#define fabs	__builtin_fabsf
#define abs	__builtin_abs

#define sind	__builtin_sin
#define cosd	__builtin_cos

#if 0
/* memset/memcpy replacements (todo?) */
extern void mem_clear( void *p, U8 b, U32 size );
extern void mem_copy( void * RESTRICT dst, void * RESTRICT src, U32 size );
#else
#  define mem_clear __builtin_memset
#  define mem_copy __builtin_memcpy
#endif

#define ASM_EXIT \
	asm( "movl $1, %eax; int $128;" ) /** Uninitialized exit code. Who cares? **/
	/* asm( "movl $1, %eax; xor %ebx, %ebx; int $128;" ) */ /** Sets exit code to 0 **/
	/* asm( "movl $1, %%eax; mov %0, %%ebx; int $128;" : : "g"((ret_num)) ) */ /** Sets exit code to a variable called ret_num */

/* Exits the program immediately */
#if USE_SDL
	#define EXIT() do { SDL_Quit(); ASM_EXIT; } while(0)
#else
	#define EXIT() ASM_EXIT
#endif

/*
asm( "movl $1, %eax; xor %ebx, %ebx; int $128;" )
*/

#define _PRINT(str,size) asm( \
	/* This writes the given characters to stdout */ \
	"mov $4, %%eax;" /* system call number */ \
	"mov $1, %%ebx;" /* file descriptor */ \
	"movl %0, %%ecx;" /* str pointer */ \
	"movl %1, %%edx;" /* str len */ \
	"int $128;" \
	: : "g"(str), "g"(size) : "eax", "ebx", "ecx", "edx" );

#define PRINT(str) _PRINT( str, sizeof(str)-1 ) /* Print string */
#define PRINT_LINE(s) PRINT( s"\n" ) /* Print string + newline */
#define PRINT_C(c) _PRINT( c, 1 ) /* Prints one character. NOTE: NO LITERAL ARGUMENT!! Must be a pointer (char*) */

#endif
