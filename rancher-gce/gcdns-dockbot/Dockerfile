FROM ubuntu
MAINTAINER docker@unxs.io
USER root
RUN apt-get update && apt-get -y -q --no-install-recommends install \
	python iputils-ping telnet net-tools libcurl4-gnutls-dev cron supervisor wget tar curl \
	&& apt-get clean \
	&& rm -r /var/lib/apt/lists/*
RUN wget --no-check-certificate https://dl.google.com/dl/cloudsdk/channels/rapid/downloads/google-cloud-sdk-140.0.0-linux-x86_64.tar.gz > /dev/null 2>&1
RUN tar xzf google-cloud-sdk-140.0.0-linux-x86_64.tar.gz > /dev/null 2>&1 && rm google-cloud-sdk-140.0.0-linux-x86_64.tar.gz
RUN mkdir -p /var/local/dockbot
COPY ./dockbot /usr/sbin/dockbot
COPY ./dockbot.cron /var/local/dockbot/dockbot.cron
COPY ./gcloudauth.sh /usr/bin/gcloudauth.sh
RUN /usr/bin/crontab /var/local/dockbot/dockbot.cron

COPY ./supervisord/supervisord.conf /etc/supervisor/supervisord.conf
CMD ["/usr/bin/supervisord","-n"]

#docker build -t unxsio/gcdns-dockbot .
#docker run --restart unless-stopped --name gcdns-dockbot --env cGCDNSProject=adhoc-dev -v /var/run/docker.sock:/var/run/docker.sock:ro -v /var/local/dockprox:/var/local/dockprox -d unxsio/gcdns-dockbot
