#ifndef __RY_LIST_H__
	#define	__RY_LIST_H__


#include "ry_def.h"



/* 单向链表 */
typedef struct list ry_slist_t;
struct list
{
	ry_slist_t          *next;
};


/* 双向链表 */
typedef struct ry_list ry_list_t;
struct ry_list
{
    ry_list_t           *next;
    ry_list_t           *prev;
};




ry_inline void ry_slist_init(ry_slist_t *slist)
{
	slist->next = RY_NULL;
}



ry_inline void ry_slist_insert_after(ry_slist_t *l, ry_slist_t *n)
{
	n->next       = l->next;   /* n指向l的后一个节点 */
	l->next       = n;         /* l指向n */
}



ry_inline void ry_slist_remove(ry_slist_t *l, ry_slist_t *n)
{
	ry_slist_t *p = l;
	while(p->next != RY_NULL)
	{
		if(p->next == n)
		{
			p->next = n->next;
			return;
		}
		p = p->next;
	}
}



/**
 * 描述：链表初始化
 *
 **/
ry_inline void ry_list_init(ry_list_t *l)
{
	l->next = l->prev = l;
}





/**
 * 描述：指定链表节点的后面插入新节点
 *
 *  l ----> ln       l ----> ln       l              ln       l               ln
 *          |   ==>          |   ==>  |              |   ==>  |               |
 *  n <-----+        n <---->+        +----> n <---->+        +<----> n <---->+
 *
 **/
ry_inline void ry_list_insert_after(ry_list_t *l, ry_list_t *n)
{
	l->next->prev = n;         /* 被当前节点的后一个节点的前手握着 */
	n->next       = l->next;   /* 后手握着当前节点的后一个节点 */
	l->next       = n;         /* 被当前节点的后手握着 */
	n->prev       = l;         /* 前手握着当前节点 */
}





/**
 * 描述：指定链表节点的前面插入新节点
 *
 * lp <---- l       lp <---- l       lp              l       lp               l
 *  |          ==>   |          ==>   |              |  ==>   |               |
 *  +-----> n        +<----> n        +<----> n <----+        +<----> n <---->+
 *
 **/
ry_inline void ry_list_insert_before(ry_list_t *l, ry_list_t *n)
{
	l->prev->next = n;
	n->prev       = l->prev;
	l->prev       = n;
	n->next       = l;
}





/**
 * 描述：删除链表中的节点
 *
 **/
ry_inline void ry_list_remove(ry_list_t *n)
{
	n->prev->next = n->next;
	n->next->prev = n->prev;
	n->next       = n->prev = n;
}













#endif
