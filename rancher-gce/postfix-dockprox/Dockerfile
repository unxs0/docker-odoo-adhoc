FROM ubuntu
MAINTAINER docker@unxs.io
USER root
RUN apt-get update && apt-get -y -q --no-install-recommends install \
	vim-tiny libsasl2-modules rsyslog postfix iputils-ping telnet net-tools libcurl4-gnutls-dev cron supervisor \
	&& apt-get clean \
	&& rm -r /var/lib/apt/lists/*
RUN mkdir -p /var/local/dockprox
COPY ./dockprox /usr/sbin/dockprox
COPY ./dockprox.cron /var/local/dockprox/dockprox.cron
COPY ./postfix/virtual_aliases.tpl /var/local/dockprox/virtual_aliases.tpl
COPY ./postfix/main.cf.tpl /var/local/dockprox/main.cf.tpl
COPY ./postfix/mailname /etc/mailname
RUN mkdir -p /opt/odoo
COPY ./postfix/openerp_mailgate.py /opt/odoo/openerp_mailgate.py
RUN /usr/bin/crontab /var/local/dockprox/dockprox.cron

COPY ./supervisord/supervisord.conf /etc/supervisor/supervisord.conf
CMD ["/usr/bin/supervisord","-n"]

#docker build -t unxsio/postfix-dockprox .
#docker run --restart unless-stopped --name postfix-dockprox -p 25:25 -v /var/run/docker.sock:/var/run/docker.sock:ro -d unxsio/postfix-dockprox
