#!/bin/bash

/root/certbot-auto --nginx --email certbot@unxs.io --agree-tos --non-interactive \
	`/usr/sbin/dockprox --certbot-domains` \
		> /tmp/certbot.log 2>&1;
