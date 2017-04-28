FROM adhoc/odoo-ar:9.0
MAINTAINER docker@unxs.io

#pre-created AdHoc Argentina Odoo 9 DB

#                "Source": "/var/lib/docker/volumes/4eb79833d5c87d1a327fa0fc36c76e7706dcc61aa17d5a0f6e476971c260466f/_data",
#                "Destination": "/opt/odoo/data",

COPY ./data /opt/odoo/

# docker build -t unxsio/odooarg .
#docker run --restart unless-stopped --environment POSTGRES_USER=odoo --environment POSTGRES_PASSWORD=odoo --name odooarg -d unxsio/odooarg
