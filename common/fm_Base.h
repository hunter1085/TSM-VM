#ifndef _FM_BASE_H_
#define _FM_BASE_H_

#include <string.h>
#include <pthread.h> 
#include "fm_List.h"

#define BIT0     0x00000001
#define BIT1     0x00000002
#define BIT2     0x00000004
#define BIT3     0x00000008
#define BIT4     0x00000010
#define BIT5     0x00000020
#define BIT6     0x00000040
#define BIT7     0x00000080
#define BIT8     0x00000100
#define BIT9     0x00000200
#define BIT10    0x00000400
#define BIT11    0x00000800
#define BIT12    0x00001000
#define BIT13    0x00002000
#define BIT14    0x00004000
#define BIT15    0x00008000
#define BIT16    0x00010000
#define BIT17    0x00020000
#define BIT18    0x00040000
#define BIT19    0x00080000
#define BIT20    0x00100000
#define BIT21    0x00200000
#define BIT22    0x00400000
#define BIT23    0x00800000
#define BIT24    0x01000000
#define BIT25    0x02000000
#define BIT26    0x04000000
#define BIT27    0x08000000
#define BIT28    0x10000000
#define BIT29    0x20000000
#define BIT30    0x40000000
#define BIT31    0x80000000

#define BITS_PER_LONG    32

#define PUBLIC
#define LOCAL static

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef pthread_mutex_t lock_t;
#define LOCK_INIT(lock)	    pthread_mutex_init(&lock,NULL)
#define LOCK(lock)	        pthread_mutex_lock(&lock)
#define UNLOCK(lock)	    pthread_mutex_unlock(&lock)
#define LOCK_DESTROY(lock)  pthread_mutex_destroy(&lock)
#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#define STRICMP(_a_,_C_,_b_) ( strcasecmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strncasecmp(_a_,_b_,_n_) _C_ 0 )
#define set_bit(x,b) (x) |= (1U<<(b))
#define clear_bit(x,b) (x) &= ~(1U<<(b))

typedef enum {
	fm_false = 0,
	fm_true = !fm_false
}fmBool;

typedef struct{
	struct list_head head;
	lock_t lock;
}list_t;
#endif