Rancher GCE Google Cloud DNS Robot
==================================

Adds public IP A record for docker containers as they are created.

Must have Google Cloud credentials and add the auth token one time.


dockbot program
----------------

Very simple unix socket reader and JSON parser are used to
system to create custom system commands.

Designed to limit dependencies and be very fast and lightweight.

In the provided case for adding GC DNS A records for new containers.

Uses io.rancher labels and other docker API data to configure.

Easy to modify
--------------

With very basic C language skills dockbot.c is easy to change to suit your own needs. You should
be able to figure out how to parse any Docker or Rancher label name/value data.

Using
-----

Edit dockbot.c then::

    make
    docker build -t gcdns-dockbot .
    docker run --restart unless-stopped --name gcdns-dockbot -p 25:25 -v /var/run/docker.sock:/var/run/docker.sock:ro -d gcdns-dockbot

Before it will be functional you will need to authorize gcloud client::

    docker exec -ti gcdns-dockbot /google-cloud-sdk/bin/gcloud auth login

Help
----

Contact support@unxs.io for free limited support.
