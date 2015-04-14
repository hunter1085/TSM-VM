#define MSG_HEAD_LEN    12
#define MSG_MAC_LEN     4
#define MSG_CRC_LEN     2

typedef enum {
	CHECK_NONE = 0,
	CHECK_MAC  = 1,
	CHECK_CRC16  = 2,
}check_level;

typedef enum {
	CRYPTO_NONE = 0,
	CRYPTO_3DES  = 1,
}crypt_level;

typedef struct {
    u8 ver;
	u8 ctrl;
	u8 s_level; //security level
	u8 i_level; //integrity level
	int seq;
	int session;
}msg_head_t;
int msg_head_handling(struct sk_buff *skb,msg_head_t *head)
{
    u8 ver,ctrl,s_level,i_level;
	int seq,session;

    if(skb->len < MSG_HEAD_LEN) return -1;
	head->ver     = skb->data[0];
	head->ctrl    = skb->data[1];
	head->s_level = skb->data[2];
	head->i_level = skb->data[3];
	head->seq     = atoi(skb->data+4);
	head->session = atoi(skb->data+8);
	return 0;
}
int msg_tail_handling(struct sk_buff *skb,int i_level)
{
    int result = 0;
	char *end = skb->data + skb->len;
    if(i_level == CHECK_MAC){
		result = (end[-4]<<24)|(end[-3]<<16)|(end[-2]<<8)|(end[-1]);
    }
	if(i_level == CHECK_MAC){
		result = (end[-2]<<8)|(end[-1]);
	}
	return result;
}
int msg_handling(struct sk_buff *skb)
{
    int ret;
	u16 crc_rcv,crc_cal;
	msg_head_t head;
	
    ret = msg_head_handling(skb,&head);
	if(ret != 0) return ret;
	switch(head.i_level){
		case CRYPTO_NONE:
			break;
		case CHECK_MAC:
			break;
		case CHECK_CRC16:
			crc_rcv = (short)msg_tail_handling(skb,head.i_level);
			crc_cal = crc16(skb->data,skb->len-MSG_CRC_LEN);
			break;
		default:
			return -1;
			break;
	}
}
void *terminal_handling_work(void* para)
{
    ag_mobile_t *agent = (ag_mobile_t *)para;
	struct sk_buff *skb;
    while(1){
		sem_wait(&agent->sem_terminal);
		skb = skb_dequeue(&agent->rq_terminal);
		if(!skb) continue;
    }
    return (void *)0;
}
void *router_handling_work(void* para)
{
    ag_mobile_t *agent = (ag_mobile_t *)para;
    while(1){
		sem_wait(&agent->sem_router);
    }
    return (void *)0;
}