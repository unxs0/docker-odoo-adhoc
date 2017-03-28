#!/bin/bash

#domains come and go
#plus we need to use a self signed cert the first time
#ssl_certificate /etc/ssl/certs/ssl-cert-snakeoil.pem;
#ssl_certificate_key /etc/ssl/private/ssl-cert-snakeoil.key;
#ssl_certificate /etc/letsencrypt/live/{{cPublicServerName}}/fullchain.pem; # managed by Certbot
#ssl_certificate_key /etc/letsencrypt/live/{{cPublicServerName}}/privkey.pem; # managed by Certbot

PATH="/root/.local/share/letsencrypt/bin/:/home/unxs/google-cloud-sdk/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin:";

cat /dev/null > /tmp/certbot.log;

for cDomain in `/usr/sbin/dockprox --certbot-domains`;do

	echo "$cDomain" >> /tmp/certbot.log;

	if [ ! -f "/etc/letsencrypt/live/$cDomain/fullchain.pem" ];then
		echo "No certbot cert: configure domain for snakeoil" >> /tmp/certbot.log;
		/usr/sbin/dockprox --snakeoil-update $cDomain >> /tmp/certbot.log 2>&1;
	else
		echo "certbot cert exists: configure domain for snakeoil" >> /tmp/certbot.log;
		/usr/sbin/dockprox --certbot-update $cDomain >> /tmp/certbot.log 2>&1;
	fi

	#maintain or install new cert
	/root/certbot-auto certonly --nginx --email certbot@unxs.io --agree-tos --non-interactive -d $cDomain >> /tmp/certbot.log 2>&1;
	if [ "$?" == "0" ];then
		echo "certbot-auto ok: configure domain for certbot" >> /tmp/certbot.log;
		/usr/sbin/dockprox --certbot-update $cDomain >> /tmp/certbot.log 2>&1;
	fi
	echo "" >> /tmp/certbot.log;
done
