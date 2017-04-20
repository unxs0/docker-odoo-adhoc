/*
 *FILE
 *PURPOSE
 *	Very simplistic hardcoded AdHoc Odoo docker data parser
 *	for creating Nginx conf.d fileS
 *AUTHOR/LEGAL
 *	(C) Gary Wallis for AdHoc Ing. S.A. 2016-2017
 *LICENSE
 *	MIT License
 *NOTES
 *	Uses Rancher VIRTUAL_HOST env
 *	as marker for containers that will be using nginx	
 *	If this env var is  to special value {io.rancher.stack.name} it will
 *	use the Rancher stack name plus the gcdns-genbot container env cGCDNSZone
 *	for the DNS hostname if they all exist.
 *	We need to create a conf.d file for each Odoo container in order
 *	to use certbot easier.
*/
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include "jsmn.h"

#include "json.h"
#include "log.h"
#include "buf.h"
#define MAXBUFLEN 102400 //tpl file size max


//Case sensitive name/value list
// "io.rancher.service.launch.config":"io.rancher.service.primary.launch.config",...
void ParseFromJsonList(char const *cList, char const *cName, char *cValue)
{
	cValue[0]=0;
	char cNamePattern[100]={""};
	sprintf(cNamePattern,"%.64s\":\"",cName);
	unsigned uNamePatternStrLen=strlen(cNamePattern);
	char *cp=strstr(cList,cNamePattern);
	if(cp!=NULL)
	{
		char *cp2=strchr(cp+uNamePatternStrLen,'\"');
		if(cp!=NULL)
		{
			*cp2=0;
			sprintf(cValue,"%.255s",cp+uNamePatternStrLen);
			*cp2='\"';
		}
	}
}//void ParseFromJsonList(char const *cEnv, char const *cName, char *cValue)


//Case sensitive ENV type list/array
// "SOMETHING=THIS","ANOTHER=THAT",...
void ParseFromJsonArray(char const *cEnv, char const *cName, char *cValue)
{
	cValue[0]=0;
	char cNamePattern[100]={""};
	sprintf(cNamePattern,"%.64s=",cName);
	unsigned uNamePatternStrLen=strlen(cNamePattern);
	char *cp=strstr(cEnv,cNamePattern);
	if(cp!=NULL)
	{
		char *cp2=strchr(cp+uNamePatternStrLen,'\"');
		if(cp!=NULL)
		{
			*cp2=0;
			sprintf(cValue,"%.255s",cp+uNamePatternStrLen);
			*cp2='\"';
		}
	}
}//void ParseFromJsonArray(char const *cEnv, char const *cName, char *cValue)


void GetDataByContainerId(char const *cId, char const *cName, char *cData)
{
	char cURL[256] = {""};
	sprintf(cURL,"http://127.0.0.1/containers/%.99s/json",cId);
	char *cJson = json_fetch_unixsock(cURL);
	jsmntok_t *tokens = json_tokenise(cJson);

	char *str;
	for(size_t i = 0, j = 1; j > 0; i++, j--)
	{
		jsmntok_t *t = &tokens[i];

		// Should never reach uninitialized tokens
		log_assert(t->start != -1 && t->end != -1);

		//Adjust j for size
		if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
		j += t->size;

		//print token strings
		if (t->type == JSMN_STRING && json_token_streq(cJson,t,(char *)cName))
		{
			jsmntok_t *t2 = &tokens[i+1];
			str = json_token_tostr(cJson, t2);
			sprintf(cData,"%.4095s",str);
		}
	}
}//void GetDataByContainerId(char const *cId, char const *cName, char *cData)

void voidCertbotDomains(char *cMode)
{
	char *cJson = json_fetch_unixsock("http://127.0.0.1/containers/json");

	jsmntok_t *tokens = json_tokenise(cJson);

	//First pass to get global base stack items.
	for(size_t i = 0, j = 1; j > 0; i++, j--)
	{
		jsmntok_t *t = &tokens[i];

		// Should never reach uninitialized tokens
		log_assert(t->start != -1 && t->end != -1);

		//Adjust j for size
		if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
		j += t->size;

		//print token strings
		if (t->type == JSMN_STRING && json_token_streq(cJson, t, "Id"))
		{
			char cSSLDomains[256]={""};
			char cSSLEmail[256]={""};
			char cData[4096]={""};

			jsmntok_t *t2 = &tokens[i+1];

			char *cID= json_token_tostr(cJson, t2);

			GetDataByContainerId(cID,"Labels",cData);
			//printf("\tcID=%s cData=%s\n",cID,cData);
			ParseFromJsonList(cData,"CERTBOT_DOMAINS",cSSLDomains);
			ParseFromJsonList(cData,"CERTBOT_EMAIL",cSSLEmail);
			if(cMode[0]=='d')
				printf("%s\n",cSSLDomains);
			else
				printf("%s\n",cSSLEmail);
		}
	}

}//void voidCertbotDomains();


int main(int iArgc, char *cArgv[])
{

	if(iArgc==2 && !strcmp(cArgv[1],"--domains"))
	{
		voidCertbotDomains("d");
		exit(0);
	}
	else if(iArgc==2 && !strcmp(cArgv[1],"--email"))
	{
		voidCertbotDomains("e");
		exit(0);
	}
	else
	{
		printf("usage: %s [--domains | --email]\n",cArgv[0]);
	}
}
