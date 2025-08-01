#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".debug_lite.data.bss")
#pragma data_seg(".debug_lite.data")
#pragma const_seg(".debug_lite.text.const")
#pragma code_seg(".debug_lite.text")
#endif
#include "app_config.h"
#include "typedef.h"

#if CONFIG_DEBUG_LITE_ENABLE

extern void putbyte(char a);

#define putbyte_lite 		putbyte

void log_putbyte(char c)
{
    putbyte_lite(c);
}

void puts_lite(const char *out)
{
    while (*out != '\0') {
        putbyte_lite(*out);
        out++;
    }
}

int printf_lite(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int ret = print(NULL, 0, format, args);
    va_end(args);

    return ret;
}

void put_buf_lite(void *_buf, u32 len)
{
    u8 *buf = (u8 *)_buf;
    printf_lite("\n0x%x\n", buf);
    for (int i = 0; i < len; i++) {
        if (((i % 16) == 0) && (i != 0)) {
            putbyte_lite('\n');
        }
        printf_lite("%x", buf[i]);
        putbyte_lite(' ');
    }
    putbyte_lite('\n');
}

#else


void puts_lite(const char *out)
{

}


void put_buf_lite(void *_buf, u32 len)
{

}


int printf_lite(const char *format, ...)
{
    return 0;
}

#endif /* #ifdef CONFIG_DEBUG_LITE_ENABLE */


