/*
 * ry_key.c
 *
 *  Created on: 2023年7月6日
 *      Author: 231
 */

#include "ry_key.h"



/* 按键控制块，主要服务于组合键 */
static ry_key_obj_t __keyObj =
{
		.sn                 = 0,
		.event_cnt          = 0,
		.down_key_cnt       = 0,
		.current_active_key = RY_NULL,
};
/* 独立按键的单向链表，用于遍历 */
static ry_slist_t __keySlist         = {.next = RY_NULL};
/* 组合按键的单向链表，用于遍历 */
static ry_slist_t __keyCompoundSlist = {.next = RY_NULL};




void ry_key_reg(ry_key_t *key,
		uint8_t valid_level,         /* 有效电平，按键激活判断 */
		uint8_t filter,              /* 电平滤波阈值 */
		uint8_t double_click_limit,  /* 双击时间的阈值 */
		uint8_t long_limit,          /* 长按的阈值 */
		uint8_t long_long_limit,     /* 超长按的阈值 */
		uint8_t (*get_level)(void))
{
	uint8_t pos;
	key->type                    = KEY_SINGLE_TYPE;
	key->status                  = KEY_IDLE_STATUS;
	key->event                   = KEY_NONE_EVENT;
	key->sn                      = __keyObj.sn++;
	key->scan.click_cnt          = 0;
	key->scan.level              = !valid_level;
	key->scan.valid_level        = valid_level;
	key->scan.filter_cnt         = 0;
	key->scan.filter_limit       = filter;
	key->scan.tick               = 0;
	key->scan.double_click_limit = double_click_limit;
	key->scan.long_limit         = long_limit;
	key->scan.long_long_limit    = long_long_limit;
	key->scan.get_level          = get_level;
	for(pos = 0; pos < KEY_NONE_EVENT; pos++)
		key->callback[pos] = RY_NULL;
	ry_slist_init(&key->slist);
	ry_slist_insert_after(&__keySlist, &key->slist);
}



void ry_key_compound_reg(ry_key_compound_t *key, callback cbk)
{
	key->type                 = KEY_COMPOUND_TYPE;
	key->status               = KEY_IDLE_STATUS;
	key->event                = KEY_NONE_EVENT;
	key->key_num              = 0;
	key->callback[0]          = cbk;
	ry_slist_init(&key->slist);
	ry_slist_insert_after(&__keyCompoundSlist, &key->slist);
}


/* 组合键添加关联的独立按键SN号，以SN大小插入 */
void ry_key_compound_insert_key_sn(ry_key_compound_t *key, uint8_t sn)
{
	uint8_t pos;
	uint8_t temp;
	if(key->key_num >= KEY_COMPOUND_NUM)
		return;
	if(0 == key->key_num)
	{
		key->key_sn[0] = sn;
	}
	else
	{
		/* 从大到小排列 */
		for(pos = 0; pos < key->key_num; pos++)
		{
			/* 重复添加某个按键 */
			if(sn == key->key_sn[pos])
				return;
			if(sn > key->key_sn[pos])
				break;
		}
		for(temp = key->key_num; temp > pos; temp--)
			key->key_sn[temp] = key->key_sn[temp - 1];
		key->key_sn[pos] = sn;
	}
	key->key_num++;
}



/* 标记当前的事件，并转移状态 */
static void __key_event_mark(ry_key_t *key, uint8_t event)
{
	key->event = event;
	if(KEY_NONE_EVENT == event)
		return;
	/* 累加所有按键的有效事件 */
	__keyObj.current_active_key = key;
	__keyObj.event_cnt++;
	if(KEY_DOWN_EVENT == event)
	{
		__keyObj.event_down_cnt++;
	}
}


static void __key_down_mark(ry_key_t *key)
{
	if(KEY_DOWN_STATUS == key->status)
	{
		if(__keyObj.down_key_cnt < KEY_COMPOUND_NUM)
		{
			/* key链表是单向链表，注册时，其sn是从大到小 */
			__keyObj.down_key_sn_buf[__keyObj.down_key_cnt] = key->sn;
		}
		__keyObj.down_key_cnt++;
	}
}


/* 调用按键相关的事件处理函数 */
void ry_key_callback(ry_key_t *key, uint8_t event)
{
	if(KEY_NONE_EVENT != event && RY_NULL != key->callback[event])
		key->callback[event](key);
	key->event = KEY_NONE_EVENT;
}


/* 独立按键电平扫描 */
void __key_level_scan(ry_key_t *key)
{
	uint8_t level = key->scan.get_level();
	if(level != key->scan.level)
	{
		if(key->scan.filter_cnt++ >= key->scan.filter_limit)
		{
			key->scan.level      = level;
			key->scan.filter_cnt = 0;
		}
	}
	else
		key->scan.filter_cnt     = 0;
}


/* 独立按键的状态机 */
uint8_t ry_key_state_machine(ry_key_t *key)
{
	__key_level_scan(key);
	key->scan.tick++;

	switch(key->status)
	{
	case KEY_IDLE_STATUS :
		if(key->scan.level == key->scan.valid_level)
		{
			__key_event_mark(key, KEY_DOWN_EVENT);
			key->status         = KEY_DOWN_STATUS;
			key->scan.tick      = 0;
			key->scan.click_cnt = 0;
		}
		break;
	case KEY_DOWN_STATUS :
		if(key->scan.level != key->scan.valid_level)
		{
			__key_event_mark(key, KEY_UP_EVENT);
			key->status         = KEY_UP_STATUS;
			if(key->scan.tick < key->scan.double_click_limit)
				key->scan.click_cnt++;
		}
		else if(key->scan.tick == key->scan.long_limit)
		{
			__key_event_mark(key, KEY_LONG_PRESS_EVENT);
			key->scan.click_cnt = 0;
		}
		else if(key->scan.tick == key->scan.long_long_limit)
		{
			__key_event_mark(key, KEY_LONG_PRESS_EVENT);
			key->scan.click_cnt = 0;
#if KEY_LONG_LONG_PRESS_STATUS_KEEP == 0
			key->status         = KEY_IDLE_STATUS;
#endif
		}
		break;
	case KEY_UP_STATUS :
		if(key->scan.level == key->scan.valid_level)
		{
			__key_event_mark(key, KEY_DOWN_EVENT);
			key->status         = KEY_DOWN_STATUS;
			key->scan.tick = 0;
		}
		/* 从按键按下开始计时，若时间超过连击时间阈值，则认为连击结束 */
		else if(key->scan.tick > key->scan.double_click_limit)
		{
			if(1 == key->scan.click_cnt)
				__key_event_mark(key, KEY_SINGLE_CLICK_EVENT);
			else if(2 == key->scan.click_cnt)
				__key_event_mark(key, KEY_DOUBLE_CLICK_EVENT);
			else if(3 == key->scan.click_cnt)
				__key_event_mark(key, KEY_THREE_CLICK_EVENT);
			key->status         = KEY_IDLE_STATUS;
		}
		break;
	default :
		key->status             = KEY_IDLE_STATUS;
		break;
	}
	__key_down_mark(key);
	return key->event;
}



/* 按键扫描，优先响应组合键，独立按键次之 */
void ry_key_scan(void)
{
	uint8_t pos;
	uint8_t Error;
	ry_key_compound_t *KeyCompound;
	ry_slist_t *Node;

	/* 扫描所有的独立物理按键 */
	for(Node = &__keySlist; Node->next != RY_NULL; Node = Node->next)
	{
		ry_key_state_machine(TASK_CONTAINER_OF(Node, ry_key_t, slist));
	}

	/* 过多按键激活 */
	if(__keyObj.event_cnt > KEY_COMPOUND_NUM || __keyObj.down_key_cnt > KEY_COMPOUND_NUM)
		goto __KEY_EVENT_CLEAR;
	/* 多个按键触发了不同的事件 */
	if(__keyObj.event_cnt > 1 && __keyObj.event_cnt != __keyObj.event_down_cnt)
		goto __KEY_EVENT_CLEAR;

	/* 需要判断组合键，高优先级。有多个独立按键激活，同时按下事件数不为0，没有发生其它事件 */
	/* 存在小概率事件，即组合键本应匹配成功，但因为其中某个关联按键的长按事件触发了，导致误判为事件冲突 */
	if(__keyObj.down_key_cnt > 1 &&
		__keyObj.event_down_cnt > 0 &&
		__keyObj.event_cnt == __keyObj.event_down_cnt)
	{
		for(Node = &__keyCompoundSlist; Node->next != RY_NULL; Node = Node->next)
		{
			KeyCompound = TASK_CONTAINER_OF(Node, ry_key_compound_t, slist);
			/* 组合键的按键个数与激活的按键个数匹配 */
			/* 该方案无组合键包含冲突，个数越多优先级越高 */
			if(KeyCompound->key_num == __keyObj.down_key_cnt)
			{
				Error = 0;
				for(pos = 0; pos < __keyObj.down_key_cnt; pos++)
				{
					/* 两个数组缓存的sn，都是递减排列 */
					if(__keyObj.down_key_sn_buf[pos] != KeyCompound->key_sn[pos])
					{
						Error = 1;
						break;
					}
				}
				/* 组合键识别到了 */
				if(0 == Error)
				{
					ry_key_callback((ry_key_t *)KeyCompound, KEY_COMPOUND_EVENT);
					break;
				}
			}
		}
	}
	/* 处理独立按键的事件，低优先级。必须要有一个事件，且已按下的按键数 <= 1 */
	else if(1 == __keyObj.event_cnt && __keyObj.down_key_cnt <= 1)
	{
		/* 在上述条件下，如果当前按键没有按下，则已按下的按键数必须为0 */
		if(KEY_DOWN_STATUS != __keyObj.current_active_key->status && 1 == __keyObj.down_key_cnt)
			goto __KEY_EVENT_CLEAR;
		ry_key_callback(__keyObj.current_active_key, __keyObj.current_active_key->event);
	}

__KEY_EVENT_CLEAR :
	__keyObj.event_cnt          = 0;
	__keyObj.event_down_cnt     = 0;
	__keyObj.down_key_cnt       = 0;
	__keyObj.current_active_key = RY_NULL;
}



