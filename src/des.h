#ifndef __DES_H
#define __DES_H

typedef unsigned int u32_t_des;


#define bit_swap(a,b,n,m)       \
    tt = ((a >> n) ^ b) & m;    \
    b ^= tt; a ^= (tt << n)

#ifndef byte
#define byte(x,n)   ((unsigned char)((x) >> (8 * (n))))
#endif

#ifdef  __cplusplus
extern "C"
{
#endif
    void des_ky(void *kval, void *key);
    void des_ec(const void *i_blk, void *o_blk, void *key);
    void des_dc(const void *i_blk, void *o_blk, void *key);
	void des_encode(const void *i_blk, void *o_blk, void *key, int len);
	void des_decode(const void *i_blk, void *o_blk, void *key, int len);
#ifdef  __cplusplus
};
#endif

#endif // __DES_H
