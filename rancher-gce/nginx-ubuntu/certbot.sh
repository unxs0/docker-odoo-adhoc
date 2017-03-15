#!/bin/bash

if [ "$CERTBOT_EMAIL" != "" ] && [ "$CERTBOT_DOMAIN" != "" ];then
	grep rancher.your.io /etc/nginx/conf.d/default.conf > /dev/null 2>&1;
	if [ "$?" == "0" ];then
		sed -i s/rancher.your.io/$CERTBOT_DOMAIN/g /etc/nginx/conf.d/default.conf;
	fi
	/root/certbot-auto --nginx --email $CERTBOT_EMAIL --agree-tos --non-interactive --domains $CERTBOT_DOMAIN > /dev/null 2>&1;
fi
