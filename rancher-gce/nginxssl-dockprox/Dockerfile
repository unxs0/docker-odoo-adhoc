FROM ubuntu
MAINTAINER docker@unxs.io
USER root
#ssl version with automated letsencrypt/certbot ssl cert management
RUN apt-get update && apt-get -y -q --no-install-recommends install \
	wget python iputils-ping telnet net-tools nginx \
	libcurl4-gnutls-dev cron supervisor \
	&& apt-get clean \
	&& rm -r /var/lib/apt/lists/*
#certbot
RUN cd /root && \
	wget --no-check-certificate https://dl.eff.org/certbot-auto && \
	chmod a+x certbot-auto
RUN /root/certbot-auto --os-packages-only --non-interactive
RUN mkdir -p /var/local/certbot
COPY ./scripts/certbot.sh /var/local/certbot/certbot.sh
#nginx
RUN mkdir -p /var/local/dockprox
COPY ./dockprox /usr/sbin/dockprox
COPY ./nginx/upstream.conf.tpl /var/local/dockprox/upstream.conf.tpl
COPY ./nginx/server.conf.tpl /var/local/dockprox/server.conf.tpl
COPY ./nginx/default.conf /etc/nginx/conf.d/default.conf
COPY ./nginx/nginx.conf /etc/nginx/nginx.conf
COPY ./nginx/index.html /usr/share/nginx/html/index.html
#cron
COPY ./cron/dockprox.cron /var/local/dockprox/dockprox.cron
COPY ./cron/certbot.cron /var/local/dockprox/certbot.cron
#RUN /usr/bin/crontab /var/local/dockprox/dockprox.cron
RUN /usr/bin/crontab /var/local/dockprox/certbot.cron

COPY ./supervisord/supervisord.conf /etc/supervisor/supervisord.conf
CMD ["/usr/bin/supervisord","-n"]

#docker build -t unxsio/nginxssl-dockprox .
#docker run --restart unless-stopped --name nginxssl-dockprox -p 80:80 -p 443:443 -v /var/run/docker.sock:/var/run/docker.sock:ro -d unxsio/nginxssl-dockprox
#docker push unxsio/nginxssl-dockprox
