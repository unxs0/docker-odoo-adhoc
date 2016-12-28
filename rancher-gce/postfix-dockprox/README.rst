Rancher GCE Postfix Odoo Docker Proxy 
=====================================

Docker image based on Ubuntu

Runs via supervisord postfix and cron. 

cron is used to reconfigure postfix if changes are picked up from the host node.

Designed for use with Rancher on a GCE Ubuntu VM.

Uses AdHoc Rancher Catalog Odoo containers what have the special ENV variables for
configuring postfix for mail proxy: VIRTUAL_ALIAS and VIRTUAL_DOMAIN


Shortly
-------

Just started nothing ready yet.


dockprox program
----------------

Very simple unix socket reader and JSON parser are used together with a simple template
system to create posfix configuration files.

Designed to limit dependencies and be very fast and lightweight.

In the provided case for an Odoo ERP postfix gateway for incoming email.

Uses io.rancher labels and other docker API data to configure.

Easy to modify
--------------

With very basic C language skills dockprox.c is easy to change to suit your own needs. You should
be able to figure out how to parse any Docker or Rancher label name/value data.

Same with the template sections and the conf .tpl files.

postfix/ contains config files and template files

Using
-----

Edit dockprox.c and .tpl files, then::

    make
    docker build -t postfix-dockprox .
    docker run --restart unless-stopped --name postfix-dockprox -p 25:25 -v /var/run/docker.sock:/var/run/docker.sock:ro -d postfix-dockprox


Help
----

Contact support@unxs.io for free limited support.
