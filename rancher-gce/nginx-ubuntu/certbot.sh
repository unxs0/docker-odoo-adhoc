#!/bin/bash

CERTBOT_EMAIL=`/usr/sbin/dockprox --email`;
CERTBOT_DOMAIN=`/usr/sbin/dockprox --domains | cut -f 1 -d ' '`;
CERTBOT_DOMAINS=`/usr/sbin/dockprox --domains`;

echo "start CERTBOT_EMAIL=$CERTBOT_EMAIL CERTBOT_DOMAIN=$CERTBOT_DOMAIN";

if [ "$CERTBOT_EMAIL" != "" ] && [ "$CERTBOT_DOMAIN" != "" ];then
	grep rancher.your.io /etc/nginx/conf.d/default.conf > /dev/null 2>&1;
	if [ "$?" == "0" ];then
		sed -i s/rancher.your.io/$CERTBOT_DOMAIN/g /etc/nginx/conf.d/default.conf;
	fi
	/root/certbot-auto --nginx --email $CERTBOT_EMAIL --agree-tos --non-interactive --domains $CERTBOT_DOMAINS > /tmp/certbot.sh.log 2>&1;
else
	echo "No CERTBOT_EMAIL CERTBOT_DOMAIN env vars" > /tmp/certbot.sh.log;
fi
