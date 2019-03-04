#!/usr/bin/env bash
set -e

if [ "$1" == "" ]
then
    echo "First argument must be a path to deb file"
    exit 255
fi

debfile=$1
fname=$(echo $debfile | awk -F "/" '{print $NF}')
curl -vvv -T $debfile -uedwardstock:$BINTRAY_API_KEY \
    -H "X-Bintray-Debian-Distribution: buster" \
    -H "X-Bintray-Debian-Component: main" \
    -H "X-Bintray-Debian-Architecture: amd64" \
    https://api.bintray.com/content/edwardstock/debian-buster/scatter/${CMAKE_PROJECT_VERSION}/pool/main/s/scatter/$fname