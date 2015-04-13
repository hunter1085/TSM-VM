#include "fm_Base.h"
#include "fm_List.h"
void list_init(list_t *list)
{
	LOCK_INIT(list->lock);
	INIT_LIST_HEAD(&list->head);
}

void list_add_entry(list_t *list,struct list_head *e)
{
    LOCK(list->lock);    
    list_add_tail(e,&list->head);
	UNLOCK(list->lock);  
}

void list_del_entry(list_t *list,struct list_head *e)
{
    LOCK(list->lock);   
    list_del(e);
	UNLOCK(list->lock);
}
void list_queue_tail(list_t *list,struct list_head *e)
{
    list_add_entry(list,e);
}
struct list_head * list_dequeue_head(list_t *list)
{
}