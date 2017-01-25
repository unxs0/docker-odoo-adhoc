/*
 *FILE
 *PURPOSE
 *	Very initial and simplistic hardcoded AdHoc Odoo docker data parser
 *	for creating system commands
 *AUTHOR/LEGAL
 *	(C) Gary Wallis for AdHoc Ing. S.A. 2016-2017
 *LICENSE
 *	MIT License
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


//Case sensitive
bool boolParseFromJsonArray(char const *cEnv, char const *cName)
{
	//printf("%s\n",cEnv);
	char cValue[8]={""};
	char cNamePattern[100]={""};
	sprintf(cNamePattern,"%.64s=",cName);
	unsigned uNamePatternStrLen=strlen(cNamePattern);
	char *cp=strstr(cEnv,cNamePattern);
	if(cp!=NULL)
	{
		//printf("cNamePattern=%s\n",cNamePattern);
		char *cp2=strchr(cp+uNamePatternStrLen,'\"');
		if(cp!=NULL)
		{
			*cp2=0;
			sprintf(cValue,"%.7s",cp+uNamePatternStrLen);
			*cp2='\"';
			//printf("%s\n",cValue);
			if(cValue[0]=='t' || cValue[0]=='T')
				return(true);
		}
	}
	return(false);
}//bool boolParseFromJsonArray(char const *cEnv, char const *cName)


//Case sensitive
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
			sprintf(cValue,"%.127s",cp+uNamePatternStrLen);
			*cp2='\"';
		}
	}
}//void ParseFromJsonArray(char const *cEnv, char const *cName, char *cValue)


void GetLabelsByContainerId(char const *cId, char *cEnv)
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
		if (t->type == JSMN_STRING && json_token_streq(cJson, t, "Env"))
		{
			jsmntok_t *t2 = &tokens[i+1];
			str = json_token_tostr(cJson, t2);
			sprintf(cEnv,"%.255s",str);
		}
	}
}//void GetLabelsByContainerId(char *cId)


int main(void)
{

	char *cJson = json_fetch_unixsock("http://127.0.0.1/containers/json");

	jsmntok_t *tokens = json_tokenise(cJson);

	char cEnv[512]={""};
	char *str;
	char cId[100]={""};
	char cContainerName[256]={""};
	char cContainerIp[256]={""};
	char cVirtualHost[256]={""};
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
			jsmntok_t *t2 = &tokens[i+1];
			str = json_token_tostr(cJson, t2);
			sprintf(cId,"%.99s",str);
			cContainerName[0]=0;
			GetLabelsByContainerId(cId,cEnv);
		}
		if (t->type == JSMN_STRING && json_token_streq(cJson, t, "io.rancher.container.ip"))
		{
       			jsmntok_t *t2 = &tokens[i+1];
			str = json_token_tostr(cJson, t2);
			sprintf(cContainerIp,"%s",str);
			char *cp;
			//chop cidr
			if((cp=strchr(cContainerIp,'/'))!=NULL)
				*cp=0;
		}
		if (t->type == JSMN_STRING && json_token_streq(cJson, t, "io.rancher.container.name"))
		{
			jsmntok_t *t2 = &tokens[i+1];
			str = json_token_tostr(cJson, t2);
			if(strstr(str,"odoo") && !strstr(str,"-db"))
			{
				sprintf(cContainerName,"%.128s",str);
				//printf("cEnv=%s\n",cEnv);
				ParseFromJsonArray(cEnv,"VIRTUAL_HOST",cVirtualHost);
				if(cVirtualHost[0])
				{
					printf("cId=%s\n",cId);
					printf("\tio.rancher.container.name=%s\n",cContainerName);
					printf("\tio.rancher.container.ip=%s\n",cContainerIp);
					printf("\tcVirtualHost=%s\n",cVirtualHost);

					char cCommand[4096]={""};
					char cPublicIp[32]={"1.2.3.4"};
					char cZone[100]={"sistemasadhoc-com"};

sprintf(cCommand,"\
	grep %2$s /tmp/alreadyadded.txt > /dev/null;\
	if [ $? = 0 ];then\
		echo %2$s already added;\
		exit 0;\
	fi;\
	\n\
	\n\
	/google-cloud-sdk/bin/gcloud dns record-sets transaction start --zone=%1$s;\
	if [ $? != 0 ];then\
		echo error1 aborting!;\
		rm transaction.yaml;\
		exit;\
	fi;\
	\n\
	\n\
	/google-cloud-sdk/bin/gcloud dns record-sets transaction add --zone=%1$s --name='public.%2$s.' --type=A --ttl=300 %3$s;\
	if [ $? != 0 ];then\
		echo error2 aborting!;\
		/google-cloud-sdk/bin/gcloud dns record-sets transaction abort --zone=%1$s;\
		exit;\
	fi;\
	\n\
	\n\
	/google-cloud-sdk/bin/gcloud dns record-sets transaction add --zone=%1$s --name='internal.%2$s.' --type=A --ttl=300 %4$s;\
	if [ $? != 0 ];then\
		echo error2 aborting!;\
		/google-cloud-sdk/bin/gcloud dns record-sets transaction abort --zone=%1$s;\
		exit;\
	fi;\
	\n\
	\n\
	/google-cloud-sdk/bin/gcloud dns record-sets transaction execute --zone=%1$s;\
	if [ $? != 0 ];then\
		echo error3 aborting!;\
		rm transaction.yaml;\
		exit;\
	fi;\
	\n\
	\n\
	echo %2$s >> /tmp/alreadyadded.txt;\
	if [ $? != 0 ];then\
		echo error4;\
	fi;\n\
		",cZone,cVirtualHost,cPublicIp,cContainerIp);

	//printf("%s",cCommand);
	system(cCommand);

				}
			}
		}
	}

	printf("Normal exit\n");
	return 0;
}//main()
