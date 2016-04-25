#include <string.h>
#include <stdio.h>

#include "elfhash.h"

char preprocess(char ch)
{
	if(ch>'9' || ch<'0')
		return ch;

	ch -= '0';

	return ch;
}

unsigned long ELFHash_o(char* str, long tablesize)  //hash函数
{  
	unsigned long hash=0;  
	unsigned long x=0;  
	unsigned int i=0;  
	unsigned int len=strlen(str);
	for(i=0;i<len;str++,i++)  //len是字符串的长度
	{  
        unsigned long ha=(hash<<4);
		
		char ch = *str;
		ch = preprocess(ch);

		hash=ha+(ch); //把字符串的ASCII值相加，并且每次把结果左移4位；
		if((x=hash&0xF0000000L)!=0)  //如果加的结果大于28位，对原结果向右移动24位 
		{	hash ^= (x >> 24); 
            hash &=~x; //与原值取异或
		}
	}  
	return hash%tablesize;  
}  


unsigned long ELFHash(char* str, long tablesize)  //hash函数
{
	str ++;
	if(*str>='4')
		str ++;

	unsigned ha = 0;

	sscanf(str, "%u", &ha);

	return ha%tablesize; 
}

