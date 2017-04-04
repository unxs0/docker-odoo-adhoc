/*
 *FILE
 *PURPOSE
 *	Very simplistic hardcoded AdHoc Odoo docker data parser
 *	for creating postfix conf files
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


int main(void)
{

	FILE *fpVirtualAliases;
	if((fpVirtualAliases=fopen("/etc/postfix/virtual_aliases.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/postfix/virtual_aliases.new\n");
		exit(2);
	}
	printf("Opened /etc/postfix/virtual_aliases.new for write\n");

	FILE *fpMainCF;
	if((fpMainCF=fopen("/etc/postfix/main.cf.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/postfix/main.cf.new\n");
		exit(3);
	}
	printf("Opened /etc/postfix/main.cf.new for write\n");

	FILE *fpVirtualDomainRegex;
	if((fpVirtualDomainRegex=fopen("/etc/postfix/virtual_domains_regex.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/postfix/virtual_domains_regex.new\n");
		exit(4);
	}
	printf("Opened /etc/postfix/virtual_domains_regex.new for write\n");

	FILE *fpEtcAliases;
	if((fpEtcAliases=fopen("/etc/aliases.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/aliases.new\n");
		exit(4);
	}
	printf("Opened /etc/aliases.new for write\n");

	FILE *fpSASLPasswd;
	if((fpSASLPasswd=fopen("/etc/postfix/sasl_passwd.new","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/postfix/sasl_passwd.new\n");
		exit(4);
	}
	printf("Opened /etc/postfix/sasl_passwd.new for write\n");

	char cMyDestination[256]={""};
	char cMyHostname[256]={""};
	char cRelayHostLine[256]={""};
	char cRelaySASLUser[256]={""};
	char cRelaySASLPasswd[256]={""};
	char cDomainsRegex1[256]={""};
	char cDomainsRegex2[256]={""};

	void voidBaseInstall(void)
	{
		printf("\tcMyHostname=%s\n",cMyHostname);
		printf("\tcMyDestination=%s\n",cMyDestination);
		if(!cMyHostname[0] || !strcmp(cMyHostname,"(null)"))
		{
			FILE *pfp;
			char cResponse[256]={""};
			if((pfp=popen("/bin/hostname -f","r"))!=NULL)
			{
				if(fscanf(pfp,"%255s",cResponse)>0)
				{
					sprintf(cMyHostname,"%.99s",cResponse);
					printf("\thostname -f cMyHostname=%s\n",cMyHostname);
				}
				pclose(pfp);
			}
			FILE *fp;
			if((fp=fopen("/etc/mailname","w"))!=NULL)
			{
				fprintf(fp,"%s\n",cMyHostname);
				fclose(fp);
			}
		}
		if(!cMyDestination[0] || !strcmp(cMyDestination,"(null)"))
			sprintf(cMyDestination,"%s,localhost",cMyHostname);
		if(!strcmp(cRelayHostLine,"(null)"))
			cRelayHostLine[0]=0;
			
		MainCfTemplate(fpMainCF,cMyHostname,cMyDestination,cRelayHostLine);
	
		printf("\tcRelayHostLine=%s\n",cRelayHostLine);
		printf("\tcRelaySASLUser=%s\n",cRelaySASLUser);
		printf("\tcRelaySASLPasswd=%s\n",cRelaySASLPasswd);
		if(cRelayHostLine[0] && cRelaySASLUser[0] && cRelaySASLPasswd[0] 
			&& strcmp(cRelayHostLine,"(null)") && strcmp(cRelaySASLUser,"(null)") && strcmp(cRelaySASLPasswd,"(null)"))
		{
			printf("\tUsing relayhost\n");
			//create SASL password file /etc/postfix/sasl_passwd entry
			fprintf(fpSASLPasswd,"%.99s %.99s:%.99s\n",cRelayHostLine,cRelaySASLUser,cRelaySASLPasswd);
			//hash it later in bash script below
		}
		else
		{
			printf("\tNot using relayhost. Missing at least 1 of 3 requirements.\n");
		}
	
		if(cDomainsRegex1[0] && strcmp(cDomainsRegex1,"(null)"))
		{
			// virtual_domains_regex
			// /[@.]adhoc\.com\.ar$/
			// /(.*)adhoc\.com\.ar$/
	
			char cDomainsRegexEscaped[512]={""};
			EscapePeriods(cDomainsRegexEscaped,cDomainsRegex1);
			fprintf(fpVirtualDomainRegex,"#\n"
					"/[@.]%s$/\n"
					"/(.*)%s$/\n",
						cDomainsRegexEscaped,
						cDomainsRegexEscaped);
		}
		if(cDomainsRegex2[0] && strcmp(cDomainsRegex2,"(null)"))
		{
			// virtual_domains_regex
			// /[@.]adhoc\.com\.ar$/
			// /(.*)adhoc\.com\.ar$/
	
			char cDomainsRegexEscaped[512]={""};
			EscapePeriods(cDomainsRegexEscaped,cDomainsRegex2);
			fprintf(fpVirtualDomainRegex,"#\n"
					"/[@.]%s$/\n"
					"/(.*)%s$/\n",
						cDomainsRegexEscaped,
						cDomainsRegexEscaped);
		}
	}//void voidBaseInstall(void)


	char *cJson = json_fetch_unixsock("http://127.0.0.1/containers/json");
	char gcGCDNSZone[256]={""};

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
			char cGCDNSZone[256]={""};
			char cServiceName[256]={""};
			char cData[4096]={""};

			jsmntok_t *t2 = &tokens[i+1];

			char *cID= json_token_tostr(cJson, t2);

			GetDataByContainerId(cID,"Labels",cData);
			ParseFromJsonList(cData,"io.rancher.stack_service.name",cServiceName);

			//gcGCDNSZone
			GetDataByContainerId(cID,"Env",cData);
			ParseFromJsonArray(cData,"cGCDNSZone",cGCDNSZone);
			if(cGCDNSZone[0] && strstr(cServiceName,"gcdns-genbot"))
			{
				//printf("\tcGCDNSZone=%s\n",cGCDNSZone);
				//printf("\tcServiceName=%s\n",cServiceName);
				sprintf(gcGCDNSZone,"%.255s",cGCDNSZone);
			}

			GetDataByContainerId(cID,"Env",cData);
			if(strstr(cServiceName,"postfix-dockprox"))
			{
				ParseFromJsonArray(cData,"cMyDestination",cMyDestination);
				ParseFromJsonArray(cData,"cRelayHostLine",cRelayHostLine);
				ParseFromJsonArray(cData,"cRelaySASLUser",cRelaySASLUser);
				ParseFromJsonArray(cData,"cRelaySASLPasswd",cRelaySASLPasswd);
				ParseFromJsonArray(cData,"cDomainsRegex1",cDomainsRegex1);
				ParseFromJsonArray(cData,"cDomainsRegex2",cDomainsRegex2);
			}
		}
	}

	voidBaseInstall();

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
			char cContainerName[256]={""};
			char cStackName[256]={""};
			char cContainerIp[256]={""};
			char cVirtualHost[256]={""};

			jsmntok_t *t2 = &tokens[i+1];
			char *cID= json_token_tostr(cJson, t2);
			//printf("%.255s\n",cID);
			char cData[4096]={""};
			GetDataByContainerId(cID,"Labels",cData);
			//printf("%.4095s\n",cData);
			ParseFromJsonList(cData,"io.rancher.container.name",cContainerName);
			ParseFromJsonList(cData,"io.rancher.stack.name",cStackName);
			//printf("\tcContainerName=%s\n",cContainerName);
			//printf("\tcStackName=%s\n",cStackName);
			ParseFromJsonList(cData,"io.rancher.container.ip",cContainerIp);
			char *cp;
			if((cp=strchr(cContainerIp,'/'))) *cp=0;
			//printf("\tcContainerIp=%s\n",cContainerIp);

			GetDataByContainerId(cID,"Env",cData);
			ParseFromJsonArray(cData,"VIRTUAL_HOST",cVirtualHost);
			//printf("\tcVirtualHost=%s\n",cVirtualHost);

			if(cVirtualHost[0])
			{
				if(cStackName[0] && gcGCDNSZone[0] && !strcmp(cVirtualHost,"{io.rancher.stack.name}"))
				{
					if((cp=strchr(gcGCDNSZone,'-'))) *cp='.';
					sprintf(cVirtualHost,"%.64s.%.128s",cStackName,gcGCDNSZone);
					printf("Using stack.name\n");
				}
				fprintf(fpVirtualAliases,"#cID=%s cContainerName=%s\n",cID,cContainerName);
				VirtualAliasesTemplate(fpVirtualAliases,cVirtualHost,cContainerName);
				printf("cID=%s\n",cID);
				printf("\tio.rancher.container.name=%s\n",cContainerName);
				printf("\tcVirtualHost=%s\n",cVirtualHost);

				// /etc/aliases.new
				char cStdDbName[256]={"default"};
				fprintf(fpEtcAliases,"#cID=%s cContainerName=%s\n",cID,cContainerName);
				fprintf(fpEtcAliases,
					"%.64s: \"| /opt/odoo/openerp_mailgate.py  --host=%.64s --port=8069 -d %.64s\"\n",
						cVirtualHost,cContainerIp,cStdDbName);
			}
		}
	}

	fclose(fpVirtualAliases);
	fclose(fpMainCF);
	fclose(fpVirtualDomainRegex);
	fclose(fpEtcAliases);
	fclose(fpSASLPasswd);

	//conditionally update and/or reload postfix
	system("\
			/usr/bin/diff -q /etc/aliases /etc/aliases.new > /dev/null;\
			if [ $? != 0 ];then\
				echo update /etc/aliases;\
				cp /etc/aliases.new /etc/aliases;\
				/usr/bin/newaliases;\
			else\
				echo /etc/aliases nothing new;\
			fi;\
			\
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
			\
			/usr/bin/diff -q  /etc/postfix/sasl_passwd /etc/postfix/sasl_passwd.new > /dev/null;\
			if [ $? != 0 ];then\
				echo update sasl_passwd;\
				cp /etc/postfix/sasl_passwd.new /etc/postfix/sasl_passwd;\
				/usr/sbin/postmap /etc/postfix/sasl_passwd;\
			else\
				echo sasl_passwd nothing new;\
			fi;\
		");

	printf("Normal exit\n");
	return 0;
}//main()
