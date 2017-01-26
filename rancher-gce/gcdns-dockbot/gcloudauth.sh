#!/bin/sh
/google-cloud-sdk/bin/gcloud auth activate-service-account --key-file=/var/local/dockprox/gc-credentials.json > /dev/null 2>&1;
/google-cloud-sdk/bin/gcloud config set project $cGCDNSProject > /dev/null 2>&1;
