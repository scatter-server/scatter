#!/usr/bin/env bash

PLATFORMS=(stretch)
PLATFORM_PACKAGE=(deb)

VM_PREFIX="scatter_build_"
VM_ID_PREFIX="scatter_"

#deb: jessie wheezy
#rpm: centos6 centos7
#???

buildRootPath="/server/_build"

for index in ${!PLATFORMS[*]}
do

    platform=${PLATFORMS[$index]}
    pkg=${PLATFORM_PACKAGE[$index]}
    outputDir=${PWD}/_build/${platform}
    vm_name="${VM_PREFIX}${platform}"
    vm_id="${VM_ID_PREFIX}${platform}"

    mkdir -p ${outputDir}

    echo " -- Build for ${platform}"
    docker build --rm -t scatter_${platform}:${index} -f ${PWD}/packaging/build_${platform}.docker ${PWD}
    containerId=$(docker run --name ${vm_name} -id ${vm_id}:${index})
    builtPackage=$(docker exec ${vm_name} ls -lsa ${buildRootPath} | grep ${pkg} | awk '{print $10}')

    echo -e "\n -- Copying ${builtPackage} to ${PWD}/_build\n"
    mkdir -p ${PWD}/_build/${platform}
    docker cp ${vm_name}:${buildRootPath}/${builtPackage} ${PWD}/_build/${platform}

    echo -e "\n -- Cleanup"
    docker stop ${containerId}

    if [ "$1" != "nodelete" ]
    then
    docker rm ${containerId}
    docker rmi ${vm_id}:${index}
    fi

    echo -e "\n"
done

# debian stretch
