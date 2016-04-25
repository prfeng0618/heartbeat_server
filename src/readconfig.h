#ifndef _READCONFIN_H
#define _READCONFIN_H



#define KEYVALLEN 100

char * l_trim(char * szOutput, const char *szInput);

char *r_trim(char *szOutput, const char *szInput);

char * a_trim(char * szOutput, const char * szInput);

int GetProfileString(char *profile, char *AppName, char *KeyName, char *KeyVal);



#endif

