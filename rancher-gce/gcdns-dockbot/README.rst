Rancher GCE Google Cloud DNS Robot
==================================

Adds public IP A record for docker containers as they are created.


dockbot program
----------------

Very simple unix socket reader and JSON parser are used to
system to create custom system commands.

Designed to limit dependencies and be very fast and lightweight.

In the provided case for adding GC DNS A records for new containers.

This is done via simple embedded /bin/sh system script.

Uses io.rancher labels, env vars and other docker API data to configure.

Easy to modify
--------------

With very basic C language and shell scripting skills required. dockbot.c is easy to change to suit your own needs. You should
be able to figure out how to parse any Docker or Rancher label name/value data.

Using
-----

Place your Google Cloud credential json file on the host node here:

    /var/local/dockprox/gc-credentials.json 

Edit dockbot.c then::

    make
    docker build -t gcdns-dockbot .
    docker run --restart unless-stopped --name gcdns-dockbot --env cGCDNSProject=myproj-dev --env cGCDNSZone=zonename-com -v /var/run/docker.sock:/var/run/docker.sock:ro -v /var/local/dockprox:/var/local/dockprox -d unxsio/gcdns-dockbot

Required environment vars::

    cGCDNSZone: This is the Google Cloud DNS zone that all operations will be performed on. This will be set via Docker compose directly or via rancher compose stack var.
    cGCDNSProject: This must match your json credentials as far as having perms to do DNS changes on cGCDNSZone.

Help
----

Contact support@unxs.io for free limited support.
