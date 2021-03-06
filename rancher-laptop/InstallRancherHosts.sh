#!/bin/bash
#Sample usage:
# ./InstallRancherHosts.sh http://192.168.99.100:8080/v1/scripts/634C4AF432A6D8B69B41:1479218400000:c6t8j4zbTKVlNI0ZXcRtzuHxos
#The arg is generated by the rancher web console Infrastructure::Hosts::Add Host page.
if [ "$1" == "" ];then
	echo "usage: $0 <agent script from rancher server add host custom command>";
	exit 0;
fi

#for cHost in host0 host1 host2;do
#for cHost in host0 host1;do
for cHost in host0 ;do
	echo $cHost;
	eval $(docker-machine env $cHost);
	docker run -d --privileged --name agent -v /var/run/docker.sock:/var/run/docker.sock -v /var/lib/rancher:/var/lib/rancher registry.unxs.io:5000/rancher/agent:v1.0.2 $1
	if [ "$?" != "0" ];then
		echo "!error run!";
	fi
	docker logs -f agent;
	docker ps --all;
done

#http://192.168.99.100:8080/v1/scripts/E6884E84931C356CC81B:1480600800000:wDrpYj63tkSlEKyEqlyyiqpqWk
