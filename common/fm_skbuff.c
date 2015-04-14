#include <pthread.h>  

#include "fm_Base.h"
#include "fm_skbuff.h"
#include "fm_Log.h"


#define SKB_ASSERT(cond)        \
    do{\
        if(!cond){\
          FM_LOGE("SKB ASSERT!");  \
          while(1);        \
        }\
    }while(0)
static inline struct sk_buff *skb_peek_tail(const struct sk_buff_head *list_)
{
	struct sk_buff *skb = list_->prev;

	if (skb == (struct sk_buff *)list_)
		skb = NULL;
	return skb;

}

static inline void __skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
	struct sk_buff *next, *prev;

	list->qlen--;
	next	   = skb->next;
	prev	   = skb->prev;
	skb->next  = skb->prev = NULL;
	next->prev = prev;
	prev->next = next;
}
void skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
    pthread_mutex_lock(&list->lock);
    __skb_unlink(skb,list);
    pthread_mutex_unlock(&list->lock);
}

static inline struct sk_buff *__skb_dequeue_tail(struct sk_buff_head *list)
{
	struct sk_buff *skb = skb_peek_tail(list);
	if (skb)
		__skb_unlink(skb, list);
	return skb;
}



static inline struct sk_buff *__skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff *skb = skb_peek(list);
	if (skb)
		__skb_unlink(skb, list);
	return skb;
}


static inline void __skb_insert(struct sk_buff *newsk,
				struct sk_buff *prev, struct sk_buff *next,
				struct sk_buff_head *list)
{
	newsk->next = next;
	newsk->prev = prev;
	next->prev  = prev->next = newsk;
	list->qlen++;
}

static inline void __skb_queue_after(struct sk_buff_head *list,
				     struct sk_buff *prev,
				     struct sk_buff *newsk)
{
	__skb_insert(newsk, prev, prev->next, list);
}

static inline void __skb_queue_head(struct sk_buff_head *list,
				    struct sk_buff *newsk)
{
	__skb_queue_after(list, (struct sk_buff *)list, newsk);
}


static inline void __skb_queue_before(struct sk_buff_head *list,
				      struct sk_buff *next,
				      struct sk_buff *newsk)
{
	__skb_insert(newsk, next->prev, next, list);
}

static inline void __skb_queue_tail(struct sk_buff_head *list,
				   struct sk_buff *newsk)
{
	__skb_queue_before(list, (struct sk_buff *)list, newsk);
}

static inline unsigned char *skb_tail_pointer(const struct sk_buff *skb)
{
	return skb->tail;
}

static inline unsigned char *__skb_pull(struct sk_buff *skb, unsigned int len)
{
	skb->len -= len;
	//SKB_ASSERT(!(skb->len < skb->data_len));
	return skb->data += len;
}

static inline fmBool skb_is_nonlinear(const struct sk_buff *skb)
{
	return (skb->data_len!=0?fm_true:fm_false);
}
static inline void skb_set_tail_pointer(struct sk_buff *skb, const int offset)
{
	skb->tail = skb->data + offset;
}
static inline void __skb_trim(struct sk_buff *skb, unsigned int len)
{
	if (!skb_is_nonlinear(skb)) {
		FM_LOGE("warning:skb data_len = 0,trim the buffer is not proper!");
		return;
	}
	skb->len = len;
	skb_set_tail_pointer(skb, len);
}
void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk)
{
	pthread_mutex_lock(&list->lock);
	__skb_queue_head(list, newsk);
	pthread_mutex_unlock(&list->lock);
}

void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
	pthread_mutex_lock(&list->lock);
	__skb_queue_tail(list, newsk);
	pthread_mutex_unlock(&list->lock);
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff *result;

	pthread_mutex_lock(&list->lock);
	result = __skb_dequeue(list);
	pthread_mutex_unlock(&list->lock);
	return result;
}
struct sk_buff *skb_dequeue_tail(struct sk_buff_head *list)
{
	struct sk_buff *result;

	pthread_mutex_lock(&list->lock);
	result = __skb_dequeue_tail(list);
	pthread_mutex_unlock(&list->lock);
	return result;
}
unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
	unsigned char *tmp = skb_tail_pointer(skb);
    SKB_ASSERT(skb_is_nonlinear(skb));
	skb->tail += len;
	skb->len  += len;
	if (skb->tail > skb->end){
        FM_LOGE("skb_over_panic!");
        while(1);
    }
	return tmp;
}

unsigned char *skb_push(struct sk_buff *skb, unsigned int len)
{
	skb->data -= len;
	skb->len  += len;
	if (skb->data<skb->head){
        FM_LOGE("skb_under_panic!");
        while(1);
    }
	return skb->data;
}

unsigned char *skb_pull(struct sk_buff *skb, unsigned int len)
{
	return (len > skb->len) ? NULL : __skb_pull(skb, len);
}


void skb_trim(struct sk_buff *skb, unsigned int len)
{
	if (skb->len > len)
		__skb_trim(skb, len);
}
inline void skb_reserve(struct sk_buff *skb, int len)
{
	skb->data += len;
	skb->tail += len;
}

struct sk_buff *skb_peek(const struct sk_buff_head *list_)
{
	struct sk_buff *skb = list_->next;

	if (skb == (struct sk_buff *)list_)
		skb = NULL;
	return skb;
}

void skb_del(struct sk_buff *skb)
{
    skb->prev->next = skb->next;
	skb->next->prev = skb->prev;
}

int glb_skb_index = 0;
struct sk_buff *alloc_skb(unsigned int size)
{
    struct sk_buff *tmp;

    tmp = (struct sk_buff *)fm_malloc(sizeof(struct sk_buff));
    if (tmp == NULL)
        return NULL;

	memset(tmp,0,sizeof(tmp));

    tmp->buff = (unsigned char *)fm_malloc(size);
    if (tmp->buff == NULL) {
        fm_free(tmp);
        return NULL;
    }

    tmp->resend_idx = 0;    /*a newly allocated skb is no way resended*/

    memset(tmp->buff,0,size);
    tmp->data_len = size;
    tmp->len      = 0;
    tmp->head     = tmp->buff;
    tmp->data     = tmp->buff;
    tmp->tail     = tmp->buff;
    tmp->end      = tmp->buff + tmp->data_len;

	tmp->index = glb_skb_index++;
	
    //FM_LOGD("skb alloc\t%d",tmp->index);
    return tmp;
}

void kfree_skb(struct sk_buff *skb)
{
	if (skb == NULL)
		return;
#if 0
	if(skb->rsp_data != NULL){
		free(skb->rsp_data);
		skb->rsp_data = NULL;
	}
#endif

    //FM_LOGD("skb free\t%d",skb->index);
    //FM_LOGD("skb free buffer");
    if(skb->buff != NULL){
        fm_free(skb->buff);
        skb->buff = NULL;
    }

    //FM_LOGD("skb free skb");
    fm_free(skb);
    skb = NULL;
}

void skb_queue_head_init(struct sk_buff_head *list)
{
	pthread_mutex_init(&list->lock,NULL);
	list->prev = list->next = (struct sk_buff *)list;
	list->qlen = 0;

}

void skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb = skb_dequeue(list)) != NULL)
		kfree_skb(skb);
}
inline int skb_queue_empty(const struct sk_buff_head *list)
{
	return list->next == (struct sk_buff *)list;
}

inline u32 skb_queue_len(const struct sk_buff_head *list_)
{
	return list_->qlen;
}

inline void skb_resend_idx_inc(struct sk_buff *skb)
{
    if(skb->resend_idx == 0xFF)
        skb->resend_idx = 0;
    else
        skb->resend_idx++;
}

inline void skb_resend_idx_rst(struct sk_buff *skb)
{
    skb->resend_idx = 0;
}
