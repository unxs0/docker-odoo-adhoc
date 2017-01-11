/*
 *FILE
 *PURPOSE
 *	Very initial and simplistic hardcoded AdHoc Odoo docker data parser
 *	for creating postfix conf files
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
#include "template.h"
#define MAXBUFLEN 102400 //tpl file size max


void AppFunctions(FILE *fp,char *cFunction)
{
}//void AppFunctions(FILE *fp,char *cFunction)


void EscapePeriods(char* cDest,const char* cSrc) 
{
	char c;

	while((c = *(cSrc++)))
	{
		switch(c)
		{
			case '.': 
				*(cDest++) = '\\';
				*(cDest++) = '.';
			break;
			default:
				*(cDest++) = c;
		}
	}
	*cDest = '\0';
}//void EscapePeriods(char* cDest,const char* cSrc)


void MainCfTemplate(FILE *fpOut,
	char const *cContainerName,
	char const *cMyHostname,
	char const *cMyDestination,
	char const *cRelayHostLine)
{
	struct t_template template;

	template.cpName[0]="cMyHostname";
	template.cpValue[0]=cMyHostname;

	template.cpName[1]="cMyDestination";
	template.cpValue[1]=cMyDestination;

	template.cpName[2]="cRelayHostLine";
	template.cpValue[2]=cRelayHostLine;

	template.cpName[3]="";//close template!


	char cTemplate[MAXBUFLEN + 1];
	FILE *fp = fopen("/var/local/dockprox/main.cf.tpl", "r");
	if(fp!=NULL)
	{
		size_t newLen=fread(cTemplate,sizeof(char),MAXBUFLEN,fp);
		if(ferror(fp)!=0)
		{
			fputs("Error reading file main.cf.tpl\n", stderr);
			exit(1);
		}
		else
		{
			cTemplate[newLen++] = '\0'; /* Just to be safe. */
		}
		fclose(fp);
	}
	else
	{
		fputs("Error opening file /var/local/dockprox/main.cf.tpl\n", stderr);
	}
	Template(cTemplate,&template,fpOut);

}//void MainCfTemplate(FILE *fpOut,...)


void VirtualAliasesTemplate(FILE *fpOut,
	char const *cVirtualDomain,
	char const *cContainer)
{
	struct t_template template;

	template.cpName[0]="cVirtualDomain";
	template.cpValue[0]=cVirtualDomain;

	template.cpName[1]="cContainer";
	template.cpValue[1]=cContainer;

	template.cpName[2]="";//close template!


	char cTemplate[MAXBUFLEN + 1];
	FILE *fp = fopen("/var/local/dockprox/virtual_aliases.tpl", "r");
	if(fp!=NULL)
	{
		size_t newLen=fread(cTemplate,sizeof(char),MAXBUFLEN,fp);
		if(ferror(fp)!=0)
		{
			fputs("Error reading file virtual_aliases.tpl\n", stderr);
			exit(1);
		}
		else
		{
			cTemplate[newLen++] = '\0'; /* Just to be safe. */
		}
		fclose(fp);
	}
	else
	{
		fputs("Error opening file /var/local/dockprox/virtual_aliases.tpl\n", stderr);
	}
	Template(cTemplate,&template,fpOut);

}//void VirtualAliasesTemplate(FILE *fpOut,...)


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

	FILE *fp;
	if((fp=fopen("/etc/postfix/virtual_aliases.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/postfix/virtual_aliases.new\n");
		exit(2);
	}
	printf("Opened /etc/postfix/virtual_aliases.new for write\n");

	FILE *fp2;
	if((fp2=fopen("/etc/postfix/main.cf.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/postfix/main.cf.new\n");
		exit(3);
	}
	printf("Opened /etc/postfix/main.cf.new for write\n");

	FILE *fp3;
	if((fp3=fopen("/etc/postfix/virtual_domains_regex.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/postfix/virtual_domains_regex.new\n");
		exit(4);
	}
	printf("Opened /etc/postfix/virtual_domains_regex for write\n");

	char cEnv[512]={""};
	char *str;
	char cId[100]={""};
	char cContainerName[256]={""};
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
					fprintf(fp,"#cId=%s cContainerName=%s\n",cId,cContainerName);
					VirtualAliasesTemplate(fp,cVirtualHost,cContainerName);
					printf("cId=%s\n",cId);
					printf("\tio.rancher.container.name=%s\n",cContainerName);
					printf("\tcVirtualHost=%s\n",cVirtualHost);

					// /[@.]adhoc\.com\.ar$/
					// /(.*)adhoc\.com\.ar$/

					char cVirtualHostEscaped[512]={""};
					EscapePeriods(cVirtualHostEscaped,cVirtualHost);
					fprintf(fp3,"#cId=%s cContainerName=%s\n"
							"/[@.]%s\n"
							"/(.*)%s\n",
								cId,cContainerName,
								cVirtualHostEscaped,
								cVirtualHostEscaped);
				}
			}
			//This really needs to be run only once on base install
			else if(strstr(str,"postfix-dockprox"))
			{
				char cMyDestination[256]={""};
				char cMyHostname[256]={""};
				char cRelayHostLine[256]={""};
				ParseFromJsonArray(cEnv,"cMyDestination",cMyDestination);
				ParseFromJsonArray(cEnv,"cMyHostname",cMyHostname);
				ParseFromJsonArray(cEnv,"cRelayHostLine",cRelayHostLine);
				sprintf(cContainerName,"%.128s",str);
				printf("cId=%s\n",cId);
				printf("\tio.rancher.container.name=%s\n",cContainerName);
				printf("\tcMyHostname=%s\n",cMyHostname);
				printf("\tcMyDestination=%s\n",cMyDestination);
				printf("\tcRelayHostLine=%s\n",cRelayHostLine);
				//printf("\tcEnv=%s\n",cEnv);
				MainCfTemplate(fp2,cContainerName,cMyHostname,cMyDestination,cRelayHostLine);
			}
		}
	}

	fclose(fp);
	fclose(fp2);
	fclose(fp3);

	//conditionally update and/or reload postfix
	system("\
			/usr/bin/diff -q /etc/postfix/virtual_aliases /etc/postfix/virtual_aliases.new > /dev/null;\
			if [ $? != 0 ];then\
				echo update virtual_aliases;\
				cp /etc/postfix/virtual_aliases.new /etc/postfix/virtual_aliases;\
				/usr/sbin/postmap /etc/postfix/virtual_aliases;\
			else\
				echo virtual_aliases nothing new;\
			fi;\
			\
			/usr/bin/diff -q /etc/postfix/virtual_domains_regex /etc/postfix/virtual_domains_regex.new > /dev/null;\
			if [ $? != 0 ];then\
				echo update virtual_domains_regex;\
				cp /etc/postfix/virtual_domains_regex.new /etc/postfix/virtual_domains_regex;\
				cReload='Yes';\
			else\
				echo virtual_domains_regex nothing new;\
			fi;\
			\
			/usr/bin/diff -q /etc/postfix/main.cf /etc/postfix/main.cf.new > /dev/null;\
			if [ $? != 0 ];then\
				echo update main.cf;\
				cp /etc/postfix/main.cf.new /etc/postfix/main.cf;\
				cReload='Yes';\
			else\
				echo main.cf nothing new;\
			fi;\
			\
                        if [ \"$cReload\" = 'Yes' ];then\
				echo reloading postfix;\
				/usr/sbin/postfix reload;\
			fi;\
		");

	printf("Normal exit\n");
	return 0;
}//main()