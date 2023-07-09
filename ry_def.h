#ifndef __RY_DEF_H__
	#define	__RY_DEF_H__




#define  RY_USE_CRITICAL_SECTION  0


#define  RY_NULL                  0

#define  RY_TICK_MAX              0xFFFFFFFF
#define  RY_TICK_MAX_DIV_2        (RY_TICK_MAX >> 1)



#define  ry_inline                static __inline
#define  ry_weak                  __attribute__((weak))






/* 指令处理后的结果 */
typedef enum
{
    COMMAND_RUN_OK               = 0x00, /* 指令运行正常 */
    COMMAND_RUN_ERR              = 0xF1, /* 指令运行异常 */
    COMMAND_PARAM_ERR            = 0xF2, /* 指令的参数非法 */
    COMMAND_M1_BUSY              = 0xFB, /* M1通信忙，正主动与从机通信中 */
    COMMAND_SLAVE_NO_ACK         = 0xFC, /* 从机无响应 */
    COMMAND_NULL                 = 0xFE, /* 指令为空 */
    COMMAND_TRANSFER_FAIL        = 0xFF, /* 通信传输异常 */
    COMMAND_FAIL_MASK            = 0xFF, /* 指令失败的掩码 */

	BMS_PROTECT_ERR_START        = 0xA0, /* 保护启动异常 */
	BMS_PROTECT_ERR_TIMEOUT      = 0xA1, /* 超时，未进保护 */
}E_MsgErr_t;




/* 32位MCU */
typedef unsigned int              ry_core_t;


#define  TASK_CONTAINER_OF(ptr, type, member) \
            ((type *)((char *)(ptr) - (ry_core_t)(&((type *)0)->member)))



#endif

