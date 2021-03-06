/*
 *FILE
 *PURPOSE
 *	Very simplistic hardcoded AdHoc Odoo docker data parser
 *	for creating Nginx conf.d file
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


void UpstreamConfTemplate(FILE *fpOut,
		char const *cUpstreamServerName,
		char const *cUpstreamIp,
		char const *cUpStreamPort)
{
	struct t_template template;

	template.cpName[0]="cUpstreamServerName";
	template.cpValue[0]=cUpstreamServerName;

	template.cpName[1]="cUpstreamIp";
	template.cpValue[1]=cUpstreamIp;

	template.cpName[2]="cUpStreamPort";
	template.cpValue[2]=cUpStreamPort;

	template.cpName[3]="";//close template!


	char cTemplate[MAXBUFLEN + 1];
	FILE *fp = fopen("/var/local/dockprox/upstream.conf.tpl", "r");
	if(fp!=NULL)
	{
		size_t newLen=fread(cTemplate,sizeof(char),MAXBUFLEN,fp);
		if(ferror(fp)!=0)
		{
			fputs("Error reading file upstream.conf.tpl\n", stderr);
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
		fputs("Error opening file /var/local/dockprox/upstream.conf.tpl\n", stderr);
	}
	Template(cTemplate,&template,fpOut);

}//void UpstreamConfTemplate(FILE *fpOut,...)


void ServerConfTemplate(FILE *fpOut,
	char const *cPublicServerName,
	char const *cUpstreamServerName,
	char const *cUpstreamServerNameChat)
{
	struct t_template template;

	template.cpName[0]="cPublicServerName";
	template.cpValue[0]=cPublicServerName;

	template.cpName[1]="cUpstreamServerName";
	template.cpValue[1]=cUpstreamServerName;

	template.cpName[2]="cUpstreamServerNameChat";
	template.cpValue[2]=cUpstreamServerNameChat;

	template.cpName[3]="";//close template!


	char cTemplate[MAXBUFLEN + 1];
	FILE *fp = fopen("/var/local/dockprox/server.conf.tpl", "r");
	if(fp!=NULL)
	{
		size_t newLen=fread(cTemplate,sizeof(char),MAXBUFLEN,fp);
		if(ferror(fp)!=0)
		{
			fputs("Error reading file server.conf.tpl\n", stderr);
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
		fputs("Error opening file /var/local/dockprox/server.conf.tpl\n", stderr);
	}
	Template(cTemplate,&template,fpOut);

}//void ServerConfTemplate(FILE *fpOut,...)


unsigned uSplitPorts(char const *cVirtualPort,char cVirtualPorts[8][32])
{
	char str[512]={""};
	sprintf(str,"%.511s",cVirtualPort);
	char ** res  = NULL;
	char *  p    = strtok (str, ",");
	int n_spaces = 0, i;

	while (p)
	{
		res = realloc (res, sizeof (char*) * ++n_spaces);
		if (res == NULL)
			exit (-1); /* memory allocation failed */
		res[n_spaces-1] = p;
		p = strtok (NULL, ",");
	}

	/* realloc one extra element for the last NULL */
	res = realloc (res, sizeof (char*) * (n_spaces+1));
	res[n_spaces] = 0;

	/* save the result */
	for (i = 0; i < (n_spaces) && i<7; ++i)
 		strncpy(cVirtualPorts[i],res[i],31);
	free (res);

	return(n_spaces);

}//unsigned uSplitPorts(char const *cVirtualPort,char cVirtualPorts[8][32])


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
	if(system("if [  -f /etc/nginx/conf.d/docker.conf ];then cp /etc/nginx/conf.d/docker.conf /etc/nginx/conf.d/docker.conf.0;fi"))
	{
		fprintf(stderr,"Could not copy /etc/nginx/conf.d/docker.conf\n");
		exit(2);
	}

	FILE *fp;
	if((fp=fopen("/etc/nginx/conf.d/docker.conf","w"))==NULL)
	{
		fprintf(stderr,"Could not open /etc/nginx/conf.d/docker.conf\n");
		exit(2);
	}
	printf("Opened /etc/nginx/conf.d/docker.conf for write\n");


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
		}
	}

	
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
			char cContainerNameChat[256]={""};
			char cStackName[256]={""};
			char cContainerIp[256]={""};
			char cVirtualHost[256]={""};
			char cVirtualPort[256]={""};

			jsmntok_t *t2 = &tokens[i+1];
			char *cID= json_token_tostr(cJson, t2);
			//printf("%.255s\n",cID);
			char cData[4096]={""};
			GetDataByContainerId(cID,"Labels",cData);
			//printf("%.4095s\n",cData);
			ParseFromJsonList(cData,"io.rancher.container.name",cContainerName);
			sprintf(cContainerNameChat,"%.248s-chat",cContainerName);
			ParseFromJsonList(cData,"io.rancher.stack.name",cStackName);
			//printf("\tcContainerName=%s\n",cContainerName);
			//printf("\tcStackName=%s\n",cStackName);
			ParseFromJsonList(cData,"io.rancher.container.ip",cContainerIp);
			char *cp;
			if((cp=strchr(cContainerIp,'/'))) *cp=0;
			//printf("\tcContainerIp=%s\n",cContainerIp);

			GetDataByContainerId(cID,"Env",cData);
			ParseFromJsonArray(cData,"VIRTUAL_PORT",cVirtualPort);
			char cVirtualPorts[8][32]={"","","","","","","",""};
			unsigned uNumPorts=0;
			uNumPorts=uSplitPorts(cVirtualPort,cVirtualPorts);
			ParseFromJsonArray(cData,"VIRTUAL_HOST",cVirtualHost);
			//printf("\tcVirtualPort=%s\n",cVirtualPort);
			//printf("\tcVirtualHost=%s\n",cVirtualHost);

			if(cVirtualHost[0])
			{

				fprintf(fp,"#cID=%s\n",cID);
				if(cStackName[0] && gcGCDNSZone[0] && (cp=strstr(cVirtualHost,"{io.rancher.stack.name}")))
				{
					char *cp2;
					//fix - this is not a complete solution, tmp hack for testing
					if((cp2=strchr(gcGCDNSZone,'-'))) *cp2='.';

					//cVirtualHost may be something like backup.{io.rancher.stack.name} handle
					if(cVirtualHost[0]=='{')
					{
						//does not have another stop
						sprintf(cVirtualHost,"%.64s.%.128s",cStackName,gcGCDNSZone);
					}
					else
					{
						//has a first part with trailing stop we hope
						*cp=0;
						char cTmp[256]={""};
						sprintf(cTmp,"%.64s%.64s.%.127s",cVirtualHost,cStackName,gcGCDNSZone);
						sprintf(cVirtualHost,"%.255s",cTmp);
					}
					fprintf(fp,"#Using stack.name\n");
					printf("Using stack.name\n");
				}
				register int n;
				for(n=0;n<uNumPorts&&n<8;n++)
				{
					if(n==0)
						UpstreamConfTemplate(fp,cContainerName,cContainerIp,cVirtualPorts[n]);
					else
						UpstreamConfTemplate(fp,cContainerNameChat,cContainerIp,cVirtualPorts[n]);
				}
				ServerConfTemplate(fp,cVirtualHost,cContainerName,cContainerNameChat);
				printf("cID=%s\n",cID);
				printf("cVirtualHost=%s\n",cVirtualHost);
				printf("cVirtualPort=%s\n",cVirtualPort);
				printf("uNumPorts=%u\n",uNumPorts);
				for(n=0;n<uNumPorts&&n<8;n++)
				{
					printf("cVirtualPorts[%d]=%s\n",n,cVirtualPorts[n]);
				}
			}
		}
	}

	fclose(fp);
	if(system("diff /etc/nginx/conf.d/docker.conf /etc/nginx/conf.d/docker.conf.0 > /dev/null 2>&1"))
	{
		if(!system("kill -HUP `pidof nginx | cut -f 2 -d ' '`"))
			printf("Reload nginx\n");
		else
			fprintf(stderr,"Reload nginx failed\n");
	}
	printf("Normal exit\n");
	return 0;
}//main()
