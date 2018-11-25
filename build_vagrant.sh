#!/usr/bin/env bash

VA_ROOT=$1 # vagrant root dir
PREFIX=$2 # projet root
OUTPUT=$3 # output dir in vagrant

VARIANT="Debug"

DEB_OS=(stretch9)
RH_OS=(centos7)

function build() {
	local boxName=$1
	local suffix=$2
	cd "${VA_ROOT}/${boxName}"
	vagrantStatus=$(vagrant status | grep running)

	echo "> Build ${boxName}"
	echo -e ${vagrantStatus}

	if [ "${vagrantStatus}" == "" ]
	then
		vagrant up
	fi

	local arch=$(vagrant ssh -c "uname -m")
	local buildDir="${PREFIX}/build/${boxName}"

	vagrant ssh -c "sudo mkdir -p ${buildDir}"

	declare -A variants
	variants=([ssl]="-DCMAKE_BUILD_TYPE=${VARIANT} -DUSE_SSL=Off" [nossl]="-DCMAKE_BUILD_TYPE=${VARIANT} -DUSE_SSL=On")

	if [ "${suffix}" == "el7" ]; then
		ext="rpm"
	else
		ext="deb"
	fi

	for var in "${variants[@]}"; do
		echo -e "> Build variant ${boxName} -> ${var}"

		vagrant ssh -c "cd ${buildDir} && cmake ../../ ${var}"
		vagrant ssh -c "cd ${buildDir} && make -j2 && sudo make install && make package"

		vers=$(vagrant ssh -c "${buildDir}/scatter -v | tr -d '\n'")
		outFile="${OUTPUT}/scatter-${vers}-linux-${arch}.${suffix}.${ext}"
		vagrant ssh -c "cp ${buildDir}/scatter-${vers}* ${outFile}"

		echo -e "> Copy package ${outFile}"
	done
}

function buildOn() {
	local arr=("$@")

	local cnt="${#arr[@]}"
	local last="${arr[$cnt-1]}"

	for((i=0; i < cnt-1; i++)); do
		boxName="${arr[$i]}"
		build "${boxName}" "${last}"
	done

}

buildOn "${DEB_OS[@]}" "stretch"
buildOn "${RH_OS[@]}" "el7"

echo -e "Done!"