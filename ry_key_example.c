/*
 * 
 */

#include "ry_key.h"

static ry_key_t __keyPower;              /* 电源按键 */
static ry_key_t __keyCtr;                /* 控制按键 */
static ry_key_compound_t __compoundKey1; /* 组合键 */


extern uint8_t key_power_get_level(void);
extern uint8_t key_ctr_get_level(void);

void key_power_long_press_callback(ry_key_t *key)
{
	printf("key_power_long_press_callback");
}
void key_ctr_single_click_callback(ry_key_t *key)
{
	printf("key_ctr_single_click_callback");
}
void compound_key1_callback(ry_key_t *key)
{
	printf("compound_key1_callback");
}

void user_key_init(void)
{
	ry_key_reg(&__keyPower, 1, 5, 50, 300, 900, key_power_get_level);
	ry_key_reg(&__keyCtr,   1, 5, 50, 300, 900, key_ctr_get_level);
	ry_key_compound_reg(&__compoundKey1, );
	
	RY_KEY_CALLBACK_CFG(__keyPower, KEY_LONG_PRESS_EVENT, key_power_long_press_callback);
	RY_KEY_CALLBACK_CFG(__keyCtr, KEY_SINGLE_CLICK_EVENT, key_ctr_single_click_callback);
	RY_KEY_CALLBACK_CFG(__compoundKey1, KEY_COMPOUND_EVENT, compound_key1_callback);
	
	ry_key_compound_insert_key_sn(&__compoundKey1, __keyPower.sn);
	ry_key_compound_insert_key_sn(&__compoundKey1, __keyCtr.sn);
}

int main(void)
{
	//system_init();
	user_key_init();
	while(1)
	{
		/* 配置定时器，定时扫描按键效果更好 */
		ry_key_scan();
	}
	
}




