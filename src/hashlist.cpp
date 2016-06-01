#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include "elfhash.h"
#include "hashlist.h"
#include "debug.h"

struct listhash * initializetable(unsigned long tablesize)
{
    struct listhash * H;

    if(tablesize <= 0)
    {
        warnTrace("%s:user cache size too small\n", __FUNCTION__);
        return NULL;
    }

    H = (struct listhash *)malloc(sizeof(struct listhash));

    if(H == NULL)
    {
        warnTrace("%s:user cache allocation fail\n", __FUNCTION__);
        return NULL;
    }

    H->tablesize = tablesize;
    H->thelist = (struct listnode * *)malloc(sizeof(struct listnode *)*
                 H->tablesize);

    if(H->thelist == NULL)
    {
        warnTrace("%s:out of space\n", __FUNCTION__);
        free(H);
        return NULL;
    }

    memset(H->thelist, 0 , sizeof(struct listnode *) * H->tablesize);

    return H;
}

struct listnode * findNode(char *str, struct listhash * H)
{
    struct listnode * listh, *p;

    listh = H->thelist[ELFHash(str, H->tablesize)];

    p = listh;

	if (p == NULL) {
		dbgTrace("%s: can't find hash list node head : %02x%02x%02x%02x%02x%02x", __FUNCTION__,
			(u8_t)str[0],(u8_t)str[1],(u8_t)str[2],(u8_t)str[3],(u8_t)str[4],(u8_t)str[5]);
		return p;
	}

#if 0
	char hexStr[6] = {0x11,0x22,0x33,0x00,0x55,0x66};
	str = hexStr;
	//if(strncmp(p->macInfo.equipmentSn, str, 6) != 0) {
	dbgTrace("%s:%d  (%04x%04x %04x%04x %04x%04x) !!\n", __FUNCTION__, __LINE__,
		*(u16_t*)(&p->macInfo.equipmentSn[0]),*(u16_t*)(&str[0]),
		*(u16_t*)(&p->macInfo.equipmentSn[2]),*(u16_t*)(&str[2]), 
		*(u16_t*)(&p->macInfo.equipmentSn[4]),*(u16_t*)(&str[4])
	);

	if( (*(u16_t*)&p->macInfo.equipmentSn[0]^*(u16_t*)&str[0] || 
		*(u16_t*)&p->macInfo.equipmentSn[2]^*(u16_t*)&str[2] || 
		*(u16_t*)&p->macInfo.equipmentSn[4]^*(u16_t*)&str[4])!= 0 ) {

		dbgTrace("%s:%d  (%02x%02x%02x%02x%02x%02x, %02x%02x%02x%02x%02x%02x, 6) is not eq !!!!\n", __FUNCTION__, __LINE__,
			p->macInfo.equipmentSn[0],p->macInfo.equipmentSn[1],p->macInfo.equipmentSn[2],
			p->macInfo.equipmentSn[3],p->macInfo.equipmentSn[4],p->macInfo.equipmentSn[5],
			(u8_t)str[0],(u8_t)str[1],(u8_t)str[2],
			(u8_t)str[3],(u8_t)str[4],(u8_t)str[5]);
	} else {
		dbgTrace("%s:%d  (%02x%02x%02x%02x%02x%02x, %02x%02x%02x%02x%02x%02x, 6) is eq !!!!\n", __FUNCTION__, __LINE__,
			p->macInfo.equipmentSn[0],p->macInfo.equipmentSn[1],p->macInfo.equipmentSn[2],
			p->macInfo.equipmentSn[3],p->macInfo.equipmentSn[4],p->macInfo.equipmentSn[5],
			(u8_t)str[0],(u8_t)str[1],(u8_t)str[2],
			(u8_t)str[3],(u8_t)str[4],(u8_t)str[5]);
	}
#endif

    while( p != NULL && (*(u16_t*)&p->macInfo.equipmentSn[0]^*(u16_t*)&str[0] || 
			*(u16_t*)&p->macInfo.equipmentSn[2]^*(u16_t*)&str[2] || 
			*(u16_t*)&p->macInfo.equipmentSn[4]^*(u16_t*)&str[4]) != 0 )
    {
        //char equipmentSn[8] = {0};
        //strncpy(equipmentSn, p->macInfo.equipmentSn, 6);
        //dbgTrace("%s:%d  equipmentSn:", __FUNCTION__, __LINE__);
        //printf_equipmentsn(p->macInfo.equipmentSn, 6);
        p = p->next;
    }

    if(p != NULL)
    {
        //char equipmentSn[8] = {0};
        //strncpy(equipmentSn, p->macInfo.equipmentSn, 6);
        dbgTrace("%s: find node equipmentSn : %02x%02x%02x%02x%02x%02x", __FUNCTION__,
        		(u8_t)p->macInfo.equipmentSn[0],(u8_t)p->macInfo.equipmentSn[1],(u8_t)p->macInfo.equipmentSn[2],
				(u8_t)p->macInfo.equipmentSn[3],(u8_t)p->macInfo.equipmentSn[4],(u8_t)p->macInfo.equipmentSn[5]);
    } else {
        dbgTrace("%s: can't find node equipmentSn : %02x%02x%02x%02x%02x%02x", __FUNCTION__,
			(u8_t)str[0],(u8_t)str[1],(u8_t)str[2],(u8_t)str[3],(u8_t)str[4],(u8_t)str[5]);
    }
    return p;
}

//如果没有就插入结点，如果有，就修改状态
int insertNode(struct listnode * istr, struct listhash * H)
{
    struct listnode * pos;
    struct listnode * newcell, *listf;
    char * str = istr->macInfo.equipmentSn;

    pos = findNode(str, H);

    if(pos != NULL)
    {
        pos->macInfo.onLineStatus = istr->macInfo.onLineStatus;
        pos->macInfo.sessionFd = istr->macInfo.sessionFd;
        return 0;
    }

    newcell = (struct listnode *)malloc(sizeof(struct listnode));

    if(newcell == NULL)
    {
        warnTrace("%s:out of space\n", __FUNCTION__);

        return 0;
    }
    else

    {
        memcpy(&newcell->macInfo, istr, sizeof(* istr));
        int idx;
        idx = ELFHash(str, H->tablesize);
        listf = (struct listnode *)H->thelist[idx];

        newcell->next = listf;
        H->thelist[idx] = newcell;
		
        dbgTrace("insert into cache %s:%d mac(%02x%02x%02x%02x%02x%02x)\n", __FUNCTION__, __LINE__,
			(u8_t)newcell->macInfo.equipmentSn[0],(u8_t)newcell->macInfo.equipmentSn[1],(u8_t)newcell->macInfo.equipmentSn[2],
			(u8_t)newcell->macInfo.equipmentSn[3],(u8_t)newcell->macInfo.equipmentSn[4],(u8_t)newcell->macInfo.equipmentSn[5]);
    }

    return 1;
}


int deleteNode(char * str, struct listhash * H)
{
    struct listnode * listh, *p, *q;
    p = q = NULL;
    listh = H->thelist[ELFHash(str, H->tablesize)];
    p = listh;

    if(p == NULL)
    {
        return 0;
    }
    else
    {
        char * cmpstr = p->macInfo.equipmentSn;

        if(strncmp(cmpstr, str, 6) == 0)
        {
            H->thelist[ELFHash(str, H->tablesize)] = p->next;

            free(p);
            p = NULL;
            return 1;
        }
        else
        {
            while(p != NULL && (strncmp(cmpstr, str, 6) != 0))
            {
                q = p;
                p = p->next;

                cmpstr = p->macInfo.equipmentSn;
            }

            if(p == NULL)
            {
                keyTrace("%s:can not find element\n", __FUNCTION__);
                return 0;
            }
            else
            {
                q->next = p->next;

                struct listnode *testp;
                testp = findNode(str, H);

                if(testp == NULL)
                {
                    dbgTrace("%s:  success delete\n", __FUNCTION__);
                }
                else
                {
                    dbgTrace("%s: p:%p\n", __FUNCTION__, testp);
                }

                free(p);

                p = NULL;

                return 1;

            }

        }

    }

    return 0;

}


int getlistlen(struct listnode * list)
{
    if(list == NULL)
        return 0;

    int n = 0;

    while(list != NULL)
    {
        n++;
        list = list->next;
    }

    return n;
}

void test1(struct listhash * H)
{
    unsigned int i;

    int blanks = 0;

    for(i = 0; i < H->tablesize; i++)
    {
        struct listnode * list = H->thelist[i];

        if(list == NULL)
            blanks ++;
    }

    keyTrace("blank: %d of %ld\n", blanks, H->tablesize);
    keyTrace("blank: %f%% \n", 100.0 * blanks / H->tablesize);
}

void test2(struct listhash * H)
{
    unsigned int i;

    int maxdup = 0;

    for(i = 0; i < H->tablesize; i++)
    {
        int len;

        struct listnode * list = H->thelist[i];
        len = getlistlen(list);

        if(maxdup < len)
            maxdup = len;
    }

    printf("max dup: %d\n", maxdup);
}

void test3(struct listhash * H)
{
    unsigned int i;

    int sum = 0;

    for(i = 0; i < H->tablesize; i++)
    {
        if(sum > 96)
            break;

        struct listnode * list = (struct listnode *)H->thelist[i];
        struct listnode * p = list;

        if(list == NULL)
            continue;

        while(p != NULL)
        {
            sum++;
            dbgTrace("%s:%d   %s  %d  %d\n", __FUNCTION__, sum, p->macInfo.equipmentSn, p->macInfo.onLineStatus, p->macInfo.sessionFd);
            p = p->next;

        }

    }

}

//调用三个测试函数
void test(struct listhash * H)
{
    test1(H);

    test2(H);

    test3(H);
}

//打印出表里面每一列的数目
void printhash(struct listhash *H)
{
    FILE * fp2;
    fp2 = fopen("D:\\c program\\3.txt", "w");

    if(fp2 == NULL)
    {
        printf("cannot open this file\n");
        exit(0);
    }

    unsigned int i;

    for(i = 0; i < H->tablesize; i++)
    {
        struct listnode *p;

        p = H->thelist[i];
        int number = 0;

        while(p != NULL)
        {
            number++;
            p = p->next;
        }

        fprintf(fp2, "%d\n", number);
    }

    fclose(fp2);
}


