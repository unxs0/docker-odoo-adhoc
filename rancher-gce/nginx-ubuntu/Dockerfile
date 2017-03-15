FROM ubuntu
MAINTAINER docker@unxs.io
USER root
RUN apt-get update && apt-get -y -q --no-install-recommends install \
	nginx python iputils-ping telnet net-tools cron supervisor wget tar \
	&& apt-get clean \
	&& rm -r /var/lib/apt/lists/*
#certbot
RUN cd /root && \
	wget --no-check-certificate https://dl.eff.org/certbot-auto && \
	chmod a+x certbot-auto
RUN /root/certbot-auto --os-packages-only --non-interactive
RUN mkdir -p /var/local/certbot
COPY ./certbot.cron /var/local/certbot/certbot.cron
COPY ./certbot.sh /var/local/certbot/certbot.sh
RUN /usr/bin/crontab /var/local/certbot/certbot.cron
#nginx
COPY nginx.conf /etc/nginx/nginx.conf
COPY cert.pem /etc/nginx/cert.pem
COPY cert.key /etc/nginx/cert.key
COPY conf.d/default.conf /etc/nginx/conf.d/default.conf
#supervisord and container entry
COPY ./supervisord/supervisord.conf /etc/supervisor/supervisord.conf
CMD ["/usr/bin/supervisord","-n"]

#docker build -t unxsio/certbot-nginx .
#docker run --restart unless-stopped -e "CERTBOT_EMAIL=certbot@unxs.io" -e "CERTBOT_DOMAIN=rancher.sistemasadhoc.com" --name certbot-nginx -p 443:443 -p 80:80 -d unxsio/certbot-nginx
