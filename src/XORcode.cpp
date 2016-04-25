#include "protype.h"
#include "xorcode.h"

//len是明文的长度
int XORencode(void *i_block,  void *o_block, u32_t key, u32_t len)
{
    u32_t *pSrc = (u32_t *)i_block;
    u32_t *pDst = (u32_t *)o_block;
    u32_t i;

    for(i = 0; i < len/sizeof(u32_t); i++)
    {
        pDst[i] = pSrc[i] ^ key;
    }

    u32_t j;
    u8_t *pKey = (u8_t *)&key;
    u8_t *p8Src = (u8_t *)&pSrc[i];
    u8_t *p8Dst = (u8_t *)&pDst[i];

    for(j = 0; j < len % sizeof(u32_t); j++)
    {
        p8Dst[j] = p8Src[j] ^ pKey[j];
    }

    return 1;
}

