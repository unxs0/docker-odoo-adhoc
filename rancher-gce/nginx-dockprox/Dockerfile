FROM ubuntu
MAINTAINER docker@unxs.io
USER root
RUN apt-get update && apt-get -y -q --no-install-recommends install iputils-ping telnet net-tools nginx libcurl4-gnutls-dev cron supervisor \
	&& apt-get clean \
	&& rm -r /var/lib/apt/lists/*
RUN mkdir -p /var/local/dockprox
COPY ./dockprox /usr/sbin/dockprox
COPY ./nginx/upstream.conf.tpl /var/local/dockprox/upstream.conf.tpl
COPY ./nginx/server.conf.tpl /var/local/dockprox/server.conf.tpl
COPY ./nginx/default.conf /etc/nginx/conf.d/default.conf
COPY ./nginx/nginx.conf /etc/nginx/nginx.conf
COPY ./nginx/index.html /usr/share/nginx/html/index.html
COPY ./dockprox.cron /var/local/dockprox/dockprox.cron
RUN /usr/bin/crontab /var/local/dockprox/dockprox.cron

COPY ./supervisord/supervisord.conf /etc/supervisor/supervisord.conf
CMD ["/usr/bin/supervisord","-n"]

#docker build -t unxsio/nginx-dockprox .
#docker run --restart unless-stopped --name nginx-dockprox -p 80:80 -v /var/run/docker.sock:/var/run/docker.sock:ro -d unxsio/nginx-dockprox
#docker push unxsio/nginx-dockprox
