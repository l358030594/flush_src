#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_common.data.bss")
#pragma data_seg(".ui_common.data")
#pragma const_seg(".ui_common.text.const")
#pragma code_seg(".ui_common.text")
#endif
/*--------------------------------------------------------------------------*/
/**@file    UI_common.c
   @brief   UI 显示公共函数
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC319N
*/
/*----------------------------------------------------------------------------*/
#include "system/includes.h"

static const u8 asc_number[] = "0123456789";    ///<0~9的ASCII码表

/*----------------------------------------------------------------------------*/
/**@brief  获取一个一位十进制的数的个位
   @param  i:输入的一个一位十进制的数
   @return 无
   @note   void itoa1(u8 i, u8 *buf)
*/
/*----------------------------------------------------------------------------*/
void itoa1(u8 i, u8 *buf)
{
    buf[0] = asc_number[i % 10];
}

/*----------------------------------------------------------------------------*/
/**@brief  获取一个2位十进制的数的各个位
   @param  i:输入的一个2位十进制的数
   @return 无
   @note   void itoa2(u8 i)
*/
/*----------------------------------------------------------------------------*/
void itoa2(u8 i, u8 *buf)
{
    buf[0] = asc_number[(i % 100) / 10];
    buf[1] = asc_number[(i % 10)];
}
/*----------------------------------------------------------------------------*/
/**@brief  获取一个3位十进制数的各个位
   @param  i:输入的一个3位十进制数
   @return 无
   @note   void itoa3(u8 i, u8 *buf)
*/
/*----------------------------------------------------------------------------*/
void itoa3(u16 i, u8 *buf)
{
    buf[0] = asc_number[(i % 1000) / 100];
    buf[1] = asc_number[(i % 100) / 10];
    buf[2] = asc_number[i % 10];
}
/*----------------------------------------------------------------------------*/
/**@brief  获取一个4位十进制的数的各个位
   @param  i:输入的一个4位十进制的数
   @return 无
   @note   void itoa4(u8 i, u8 *buf)
*/
/*----------------------------------------------------------------------------*/
void itoa4(u16 i, u8 *buf)
{
    buf[0] = asc_number[(i % 10000) / 1000]; //千
    buf[1] = asc_number[(i % 1000) / 100]; //百
    buf[2] = asc_number[(i % 100) / 10]; //十
    buf[3] = asc_number[i % 10]; //个
}


