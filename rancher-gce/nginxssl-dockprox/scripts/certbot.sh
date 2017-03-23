#!/bin/bash

#domains come and go
#plus we need to use a self signed cert the first time
#ssl_certificate /etc/ssl/certs/ssl-cert-snakeoil.pem;
#ssl_certificate_key /etc/ssl/private/ssl-cert-snakeoil.key;
#ssl_certificate /etc/letsencrypt/live/{{cPublicServerName}}/fullchain.pem; # managed by Certbot
#ssl_certificate_key /etc/letsencrypt/live/{{cPublicServerName}}/privkey.pem; # managed by Certbot


for cDomain in `/usr/sbin/dockprox --certbot-domains`;do

	if [ ! -f "/etc/letsencrypt/live/$cDomain/fullchain.pem" ];then
		#configure domain for snakeoil
		/usr/sbin/dockprox --snakeoil-update $cDomain;
	fi
	/root/certbot-auto certonly --nginx --email certbot@unxs.io --agree-tos --non-interactive -d $cDomain > /tmp/certbot.log 2>&1;
	if [ "$?" == "0" ];then
		#configure domain for certbot but only if the conf.d/ file is different
		#do we reload nginx
		/usr/sbin/dockprox --certbot-update $cDomain;
	fi
done
