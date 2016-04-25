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

unsigned long ELFHash_o(char* str, long tablesize)  //hash����
{  
	unsigned long hash=0;  
	unsigned long x=0;  
	unsigned int i=0;  
	unsigned int len=strlen(str);
	for(i=0;i<len;str++,i++)  //len���ַ����ĳ���
	{  
        unsigned long ha=(hash<<4);
		
		char ch = *str;
		ch = preprocess(ch);

		hash=ha+(ch); //���ַ�����ASCIIֵ��ӣ�����ÿ�ΰѽ������4λ��
		if((x=hash&0xF0000000L)!=0)  //����ӵĽ������28λ����ԭ��������ƶ�24λ 
		{	hash ^= (x >> 24); 
            hash &=~x; //��ԭֵȡ���
		}
	}  
	return hash%tablesize;  
}  


unsigned long ELFHash(char* str, long tablesize)  //hash����
{
	str ++;
	if(*str>='4')
		str ++;

	unsigned ha = 0;

	sscanf(str, "%u", &ha);

	return ha%tablesize; 
}

