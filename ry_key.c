/*
 * ry_key.c
 *
 *  Created on: 2023年7月6日
 *      Author: 231
 */

#include "ry_key.h"

/*
 * 控制逻辑
 *
 *  1. 组合键匹配成功，则不执行其余独立按键的事件，只响应组合键
 *  2. 组合键匹配的必要条件，至少有一个关联按键触发了 KEY_DOWN_EVENT，即按下事件
 *  3. 组合键匹配失败，则响应所有独立按键触发的事件
 *
 */



/* 按键控制块，主要服务于组合键 */
static ry_key_obj_t __keyObj =
{
        .sn                 = 0,
        .event_down_cnt     = 0,
        .down_key_cnt       = 0,
};
/* 独立按键的单向链表，用于遍历 */
static ry_slist_t __keySlist         = {.next = RY_NULL};
/* 组合按键的单向链表，用于遍历 */
static ry_slist_t __keyCompoundSlist = {.next = RY_NULL};




void ry_key_reg(ry_key_t *key,
        uint8_t valid_level,         /* 有效电平，按键激活判断 */
        uint8_t keep_trigger,        /* 长按连续触发 */
#if KEY_LIMIT_DEFAULT_ENABLE == 0
        uint8_t filter,              /* 电平滤波阈值 */
        uint8_t double_click_limit,  /* 双击时间的阈值 */
        uint8_t long_limit,          /* 长按的阈值 */
        uint8_t long_long_limit,     /* 超长按的阈值 */
#endif
        uint8_t (*get_level)(void))
{
    uint8_t pos;
    key->type                    = KEY_SINGLE_TYPE;
    key->status                  = KEY_UP_STATUS;
    key->event                   = KEY_NONE_EVENT;
    key->sn                      = __keyObj.sn++;
    key->long_press_keep_trigger = keep_trigger;
    key->scan.click_cnt          = 0;
    key->scan.level              = !valid_level;
    key->scan.valid_level        = valid_level;
    key->scan.filter_cnt         = 0;
    key->scan.tick               = 0;
#if KEY_LIMIT_DEFAULT_ENABLE == 0
    key->scan.filter_limit       = filter;
    key->scan.double_click_limit = double_click_limit;
    key->scan.long_limit         = long_limit;
    key->scan.long_long_limit    = long_long_limit;
#endif
    key->scan.get_level          = get_level;
    for(pos = 0; pos < KEY_NONE_EVENT; pos++)
        key->callback[pos] = RY_NULL;
    ry_slist_init(&key->slist);
    ry_slist_insert_after(&__keySlist, &key->slist);
}



/* 组合键的注册函数 */
void ry_key_compound_reg(ry_key_compound_t *key, callback cbk)
{
    key->type                 = KEY_COMPOUND_TYPE;
    key->status               = KEY_UP_STATUS;
    key->event                = KEY_NONE_EVENT;
    key->key_num              = 0;
    key->callback[0]          = cbk;
    ry_slist_init(&key->slist);
    ry_slist_insert_after(&__keyCompoundSlist, &key->slist);
}


/* 组合键添加关联的独立按键SN号，以SN大小插入 */
void ry_key_compound_insert_key_sn(ry_key_compound_t *key, ry_key_t *k)
{
    uint8_t pos;
    uint8_t temp;
    if(key->key_num >= KEY_COMPOUND_NUM)
        return;
    if(0 == key->key_num)
    {
        key->key_sn[0] = k->sn;
    }
    else
    {
        /* 从大到小排列 */
        for(pos = 0; pos < key->key_num; pos++)
        {
            /* 重复添加某个按键 */
            if(k->sn == key->key_sn[pos])
                return;
            if(k->sn > key->key_sn[pos])
                break;
        }
        for(temp = key->key_num; temp > pos; temp--)
            key->key_sn[temp] = key->key_sn[temp - 1];
        key->key_sn[pos] = k->sn;
    }
    key->key_num++;
}



/* 标记当前的事件 */
static void __key_event_mark(ry_key_t *key, uint8_t event)
{
    key->event = event;
    /* 记录非连续触发的按下事件个数 */
    if(KEY_DOWN_EVENT == event && key->scan.tick == 0)
    {
        __keyObj.event_down_cnt++;
    }
}

/* 记录按键按下的个数 */
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
        if(++key->scan.filter_cnt >= __KEY_LEVEL_FILTER_LIMIT)
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
    /* 计时大于超长按阈值，就不要累加时间了 */
    if(key->scan.tick <= __KEY_LONG_LONG_LIMIT)
        key->scan.tick++;

    switch(key->status)
    {
    case KEY_DOWN_STATUS :
        /* 弹起事件 */
        if(key->scan.level != key->scan.valid_level)
        {
            __key_event_mark(key, KEY_UP_EVENT);
            key->status         = KEY_UP_STATUS;
            /* 记录连击次数 */
            if(key->scan.tick < __KEY_DOUBLE_CLICK_LIMIT)
                key->scan.click_cnt++;
        }
        /* 时基 >= 长按连续触发的阈值 */
        else if(key->scan.tick >= __KEY_LONG_PRESS_KEEP_TRIGGER_LIMIT)
        {
            /* 连续触发 */
            if(key->long_press_keep_trigger)
            {
                __key_event_mark(key, KEY_DOWN_EVENT);
                /* 时基清零，保证周期性触发 */
                key->scan.tick  = 0;
            }
            /* 长按事件 */
            else if(key->scan.tick == __KEY_LONG_LIMIT)
                __key_event_mark(key, KEY_LONG_PRESS_EVENT);
            /* 超长按事件 */
            else if(key->scan.tick == __KEY_LONG_LONG_LIMIT)
                __key_event_mark(key, KEY_LONG_LONG_PRESS_EVENT);
            /* 长按，则连击事件应该消除 */
            key->scan.click_cnt = 0;
        }
        break;
    case KEY_UP_STATUS :
        /* 按下事件 */
        if(key->scan.level == key->scan.valid_level)
        {
            /* 先清零，表明是非连续触发的按下事件 */
            key->scan.tick      = 0;
            __key_event_mark(key, KEY_DOWN_EVENT);
            key->status         = KEY_DOWN_STATUS;
        }
        /* 从按键按下开始计时，若时间超过连击时间阈值，则认为连击结束 */
        else if(key->scan.tick > __KEY_DOUBLE_CLICK_LIMIT && key->scan.click_cnt > 0)
        {
            if(1 == key->scan.click_cnt)
                __key_event_mark(key, KEY_SINGLE_CLICK_EVENT);
            else if(2 == key->scan.click_cnt)
                __key_event_mark(key, KEY_DOUBLE_CLICK_EVENT);
            else if(3 == key->scan.click_cnt)
                __key_event_mark(key, KEY_THREE_CLICK_EVENT);
            key->scan.click_cnt = 0;
        }
        break;
    default :
        key->status             = KEY_UP_STATUS;
        break;
    }
    __key_down_mark(key);
    return key->event;
}



/* 按键扫描，优先响应组合键，独立按键次之 */
void ry_key_scan(void)
{
    uint8_t pos;
    uint8_t FlagKeyCompound = 0;
    ry_key_t          *Key;
    ry_key_compound_t *KeyCompound;
    ry_slist_t *Node;

    /* 扫描所有的独立物理按键 */
    for(Node = __keySlist.next; Node != RY_NULL; Node = Node->next)
    {
        ry_key_state_machine(TASK_CONTAINER_OF(Node, ry_key_t, slist));
    }

    /* 过多按键激活 */
    if(__keyObj.down_key_cnt > KEY_COMPOUND_NUM)
        goto __KEY_EVENT_CLEAR;

    /* 匹配组合键，成功则不执行余下的独立按键的事件 */
    if(__keyObj.down_key_cnt > 1 && __keyObj.event_down_cnt > 0)
    {
        for(Node = __keyCompoundSlist.next; Node != RY_NULL; Node = Node->next)
        {
            KeyCompound = TASK_CONTAINER_OF(Node, ry_key_compound_t, slist);
            if(KeyCompound->key_num != __keyObj.down_key_cnt)
                continue;
            /* 组合键的按键个数与激活的按键个数匹配 */
            FlagKeyCompound = 1;
            for(pos = 0; pos < __keyObj.down_key_cnt; pos++)
            {
                /* 两个数组缓存的sn，都是递减排列 */
                if(__keyObj.down_key_sn_buf[pos] != KeyCompound->key_sn[pos])
                {
                    FlagKeyCompound = 0;
                    break;
                }
            }
            /* 组合键识别到了 */
            if(1 == FlagKeyCompound)
            {
                ry_key_callback((ry_key_t *)KeyCompound, KEY_COMPOUND_EVENT);
                break;
            }
        }
    }
    
    /* 扫描所有的独立物理按键，执行已触发的事件 */
    if(0 == FlagKeyCompound)
    {
        for(Node = __keySlist.next; Node != RY_NULL; Node = Node->next)
        {
            Key = TASK_CONTAINER_OF(Node, ry_key_t, slist);
            ry_key_callback(Key, Key->event);
        }
    }
    else
    {
        /* 清除所有独立按键的事件 */
__KEY_EVENT_CLEAR :
        for(Node = __keySlist.next; Node != RY_NULL; Node = Node->next)
        {
            TASK_CONTAINER_OF(Node, ry_key_t, slist)->event = KEY_NONE_EVENT;
        }
    }

    __keyObj.event_down_cnt     = 0;
    __keyObj.down_key_cnt       = 0;
}



