/*
 * ry_key.h
 *
 *  Created on: 2023年7月6日
 *      Author: 231
 */

#ifndef RY_TICK_TASK_RY_KEY_H_
#define RY_TICK_TASK_RY_KEY_H_

#include <stdint.h>

#include "ry_def.h"
#include "ry_list.h"



/* 按键持续按下，则保持超长按状态 */
#define KEY_LONG_LONG_PRESS_STATUS_KEEP           1

/* 独立按键扫描的阈值是否采用默认值 */
#define KEY_LIMIT_DEFAULT_ENABLE                  1



#if KEY_LIMIT_DEFAULT_ENABLE == 1
#define __KEY_LEVEL_FILTER_LIMIT                  5
#define __KEY_DOUBLE_CLICK_LIMIT                  150
#define __KEY_LONG_LIMIT                          2000
#define __KEY_LONG_LONG_LIMIT                     5000
#else
#define __KEY_LEVEL_FILTER_LIMIT                  key->scan.filter_limit
#define __KEY_DOUBLE_CLICK_LIMIT                  key->scan.double_click_limit
#define __KEY_LONG_LIMIT                          key->scan.long_limit
#define __KEY_LONG_LONG_LIMIT                     key->scan.long_long_limit
#endif


#define KEY_COMPOUND_NUM               5

typedef enum
{
	KEY_DOWN_STATUS,             /* 按下状态 */
	KEY_UP_STATUS,               /* 弹起状态 */
	KEY_IDLE_STATUS,             /* 空闲状态 */
}E_key_status_t;

typedef enum
{
	KEY_COMPOUND_EVENT,        /* 组合键事件 */
	KEY_DOWN_EVENT,            /* 按下事件 */
	KEY_UP_EVENT,              /* 弹起事件 */
	KEY_SINGLE_CLICK_EVENT,    /* 单击事件 */
	KEY_DOUBLE_CLICK_EVENT,    /* 双击事件 */
	KEY_THREE_CLICK_EVENT,     /* 三击事件 */
	KEY_LONG_PRESS_EVENT,      /* 长按事件 */
	KEY_LONG_LONG_PRESS_EVENT, /* 超长按事件 */
	KEY_NONE_EVENT,            /* 无 */
}E_key_event_t;

typedef enum
{
	KEY_SINGLE_TYPE,
	KEY_COMPOUND_TYPE,
}E_key_type_t;


typedef struct ry_key ry_key_t;
typedef void (*callback)(ry_key_t* key);


/* 独立按键扫描参数的结构体 */
typedef struct
{
	uint8_t                 level       : 1;                   /* 当前电平 */
	uint8_t                 valid_level : 1;                   /* 有效电平，按键激活判断 */
	uint8_t                 reserve     : 2;                   /* 保留 */
	uint8_t                 click_cnt   : 4;                   /* 连击次数 */
	uint8_t                 filter_cnt;                        /* 电平滤波计数 */
#if KEY_LIMIT_DEFAULT_ENABLE == 0
	uint8_t                 filter_limit;                      /* 电平滤波阈值 */
#endif
	uint16_t                tick;                              /* 按键扫描计时 */
#if KEY_LIMIT_DEFAULT_ENABLE == 0
	uint16_t                double_click_limit;                /* 双击时间的阈值 */
	uint16_t                long_limit;                        /* 长按的阈值 */
	uint16_t                long_long_limit;                   /* 超长按的阈值 */
#endif
	uint8_t               (*get_level)(void);                  /* 电平获取，函数指针 */
}key_scan_t;

/* 组合键的控制块 */
typedef struct
{
	uint8_t                 type        : 1;
	uint8_t                 status      : 3;
	uint8_t                 event       : 4;
	ry_slist_t              slist;
	callback                callback[1];
	uint8_t                 key_num;                           /* 关联的独立按键个数 */
	uint8_t                 key_sn[KEY_COMPOUND_NUM];          /* 关联的独立按键的SN号 */
}ry_key_compound_t;

/* 独立按键的控制块 */
struct ry_key
{
	uint8_t                 type        : 1;
	uint8_t                 status      : 3;
	uint8_t                 event       : 4;
	ry_slist_t              slist;
	callback                callback[KEY_NONE_EVENT];          /* 事件的回调函数 */
	uint8_t                 sn;                                /* 与注册到链表的先后序号对应 */
	key_scan_t              scan;                              /* 独立按键扫描参数的结构体 */
};


typedef struct
{
	uint8_t                 sn;                                /* 独立按键链表的下标，作为按键的SN号 */
	uint8_t                 event_cnt;                         /* 记录有多少事件触发 */
	uint8_t                 event_down_cnt;                    /* 记录有多少按压类事件触发 */
	uint8_t                 down_key_cnt;                      /* 记录有多少个按键按下 */
	uint8_t                 down_key_sn_buf[KEY_COMPOUND_NUM]; /* 缓存已按下的按键的SN号 */
	ry_key_t               *current_active_key;                /* 当前激活的按键 */
}ry_key_obj_t;



/* 注册按键的回调函数 */
#define RY_KEY_SET_EVENT_HANDLER(key, event, cbk)       key.callback[event] = cbk

extern void ry_key_reg(ry_key_t *key,
		uint8_t valid_level,         /* 有效电平，按键激活判断 */
#if KEY_LIMIT_DEFAULT_ENABLE == 0
		uint8_t filter,              /* 电平滤波阈值 */
		uint8_t double_click_limit,  /* 双击时间的阈值 */
		uint8_t long_limit,          /* 长按的阈值 */
		uint8_t long_long_limit,     /* 超长按的阈值 */
#endif
		uint8_t (*get_level)(void));
extern void ry_key_compound_reg(ry_key_compound_t *key, callback cbk);

extern void ry_key_compound_insert_key_sn(ry_key_compound_t *key, uint8_t sn);

extern void ry_key_callback(ry_key_t *key, uint8_t event);
extern uint8_t ry_key_state_machine(ry_key_t *key);

extern void ry_key_scan(void);




#endif /* RY_TICK_TASK_RY_KEY_H_ */
