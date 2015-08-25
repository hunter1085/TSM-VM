/*
 *	Definitions for the 'struct sk_buff' memory handlers.
 *
 *	Authors:
 *		Alan Cox, <gw4pts@gw4pts.ampr.org>
 *		Florian La Roche, <rzsfl@rz.uni-sb.de>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#ifndef _LINUX_SKBUFF_H
#define _LINUX_SKBUFF_H

#include "fm_Bytes.h"

struct sk_buff_head {
	struct sk_buff	*next;
	struct sk_buff	*prev;

	unsigned int     qlen;
	pthread_mutex_t	lock;
};

struct sk_buff {
	/* These two members must be first. */
	struct sk_buff		*next;
	struct sk_buff		*prev;

	int index;  //for mem-leakage debug
	fmBool isNew;
	int session;
	int msg_type;
	int reason;
	fmBytes *agent;

    unsigned char        resend_idx; /*record the re-send times*/

	unsigned int		len;        /*len = tail-data+1*/
    unsigned int        data_len;   /*data_len = buff size*/

    unsigned char       *buff;
	void                *extra;     /*for addtinal info*/
	unsigned int        rsp_data_len;

	unsigned char		*tail;
	unsigned char		*end;
	unsigned char		*head;
    unsigned char       *data;


};

#define skb_queue_walk(queue, skb) \
		for (skb = (queue)->next;					\
		     skb != (struct sk_buff *)(queue);				\
		     skb = skb->next)

#define skb_queue_walk_safe(queue, skb, tmp)					\
                for (skb = (queue)->next, tmp = skb->next;          \
                     skb != (struct sk_buff *)(queue);              \
                     skb = tmp, tmp = skb->next)

#define skb_queue_reverse_walk_safe(queue, skb, tmp)				\
                            for (skb = (queue)->prev, tmp = skb->prev;          \
                                 skb != (struct sk_buff *)(queue);              \
                                 skb = tmp, tmp = skb->prev)


void skb_unlink(struct sk_buff *skb, struct sk_buff_head *list);

void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk);

void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk);

struct sk_buff *skb_dequeue(struct sk_buff_head *list);

struct sk_buff *skb_dequeue_tail(struct sk_buff_head *list);

unsigned char *skb_put(struct sk_buff *skb, unsigned int len);

unsigned char *skb_push(struct sk_buff *skb, unsigned int len);

unsigned char *skb_pull(struct sk_buff *skb, unsigned int len);

void skb_trim(struct sk_buff *skb, unsigned int len);

inline void skb_reserve(struct sk_buff *skb, int len);

struct sk_buff *skb_peek(const struct sk_buff_head *list_);

struct sk_buff *alloc_skb(unsigned int size);

void kfree_skb(struct sk_buff *skb);

void skb_del(struct sk_buff *skb);

void skb_queue_head_init(struct sk_buff_head *list);

void skb_queue_purge(struct sk_buff_head *list);

inline int skb_queue_empty(const struct sk_buff_head *list);

inline unsigned int skb_queue_len(const struct sk_buff_head *list_);

inline void skb_resend_idx_inc(struct sk_buff *skb);

inline void skb_resend_idx_rst(struct sk_buff *skb);

#endif	/* _LINUX_SKBUFF_H */
