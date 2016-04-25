#ifndef __HASHLIST_H
#define __HASHLIST_H

#include "protype.h"

/*�û������е�һ����¼*/
struct listnode
{
    struct mac_info macInfo;
    struct listnode *next;
};

/*�û�������׵�ַ��ɢ�б��С*/
struct listhash
{
    unsigned long tablesize;
    struct listnode ** thelist;
};


/*��ʼ���û������б�*/
struct listhash * initializetable(unsigned long tablesize);


/*����һ�����ݵ��û�����*/
int insertNode(struct listnode * istr, struct listhash * H);


/*���һ������Ƿ����û�*/
struct listnode * findNode(char *str, struct listhash * H);


/*���û��������ɾ��һ����¼*/
int  deleteNode(char * str, struct listhash * H);

void printhash(struct listhash *H);

void test(struct listhash * H);

void test3(struct listhash * H);


int printf_equipmentsn(char *src, int len);
#endif

