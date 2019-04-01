#!/bin/bash

_pwd=$(pwd)

set -e

POLYCUBE_CODEGEN_LOG=/var/log/polycube/polycube-codegen.log
SWAGGER_CONFIG_FILE=$HOME/.config/polycube/swagger_codegen_config.json

function exit_error() {
	if [ "$?" -ne "0" ]; then
		echo "Failed to create the C++ stub"
		cat $POLYCUBE_CODEGEN_LOG
	fi
	exit 0
}

function show_help() {
usage="$(basename "$0") [-h] [-i input_yang] [-o output_folder] [-s output_swagger_file]
Polycube code generator that translates a YANG file into an polycube C++ service stub

where:
    -h  show this help text
    -i  path to the input YANG file
    -o  path to the destination folder where the service stub will be placed
    -s  path to the destination swagger file (optional)"

echo "$usage"
}

trap exit_error EXIT

if [ -z ${POLYCUBE_HOME+x} ]; then
	echo "\$POLYCUBE_HOME is not set"
	exit 1
else
	echo "POLYCUBE_HOME is set to '$POLYCUBE_HOME'"
fi

while getopts :i:o:s:h option; do
 case "${option}" in
 h|\?)
	show_help
	exit 0
	;;
 i) YANG_PATH=${OPTARG}
	;;
 o) OUT_FOLDER=${OPTARG}
	;;
 s) OUT_SWAGGER_PATH=${OPTARG}
 	;;
 :)
    echo "Option -$OPTARG requires an argument." >&2
    show_help
    exit 0
    ;;
 esac
done

if [ -f $POLYCUBE_CODEGEN_LOG ]; then
	rm $POLYCUBE_CODEGEN_LOG
fi

if [ -z ${YANG_PATH+x} ]; then
	echo "You should specify the YANG file with the -i option" >&2;
	show_help
	exit 0
fi

if [ -z ${OUT_FOLDER+x} ]  && [ -z ${OUT_SWAGGER_PATH+x} ]; then
	echo "You should specify the output folder file with the -o option" >&2;
	show_help
	exit 0
fi

now="$(date '+%Y_%m_%d_%H_%M_%S')"
json_filename="$now"_api.json
#echo "$now"

pyang -f swagger -p $POLYCUBE_HOME/src/services/datamodel-common $YANG_PATH -o /tmp/"$json_filename" > $POLYCUBE_CODEGEN_LOG 2>&1

if [ -n "${OUT_SWAGGER_PATH+set}" ]; then
	cp /tmp/"$json_filename" $OUT_SWAGGER_PATH
	echo "Swagger file saved in $OUT_SWAGGER_PATH"
	exit 0
fi

if [ -f $SWAGGER_CONFIG_FILE ]; then
	java -jar $POLYCUBE_HOME/swagger-codegen-cli.jar generate -l polycube -i /tmp/"$json_filename" -o $OUT_FOLDER --config $SWAGGER_CONFIG_FILE > $POLYCUBE_CODEGEN_LOG 2>&1
else
	java -jar $POLYCUBE_HOME/swagger-codegen-cli.jar generate -l polycube -i /tmp/"$json_filename" -o $OUT_FOLDER > $POLYCUBE_CODEGEN_LOG 2>&1
fi


rm /tmp/"$json_filename"

cd $_pwd

echo "C++ service stub generated under $OUT_FOLDER/src"
exit 0


