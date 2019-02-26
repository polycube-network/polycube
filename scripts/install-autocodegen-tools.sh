#!/bin/bash

# This script installs the tools needed to automatically generate the
# code for a new service, together with their dependencies.
# In particular, it installs the pyang-swagger tool that is used to
# generate the swagger-compatible REST APIs and the swagger-codegen
# tool that is used to generate the service stub.
#
# In addition, this script installs the polycube-codegen application that
# can be used to interact with the aforementioned tools and generate the
# service stub and/or the REST APIs given the YANG model of the new service.
#
# Run the polycube-codegen application form the terminal after executing this
# script in order to get the needed information.
#
# The script will download the tools if they are not installed, while it will
# update those tools otherwise.

function success_message {
  set +x
  echo
  echo 'Installation completed successfully'
}

_pwd=$(pwd)

set -e

if [ -z ${POLYCUBE_HOME+x} ]; then
    echo "\$POLYCUBE_HOME is not set"
    exit 1
else
    echo "POLYCUBE_HOME is set to '$POLYCUBE_HOME'"
fi

echo "Install pyang-swagger dependencies"
if [ ! -d "$HOME/dev" ]; then
    mkdir $HOME/dev
fi

cd $HOME/dev

APT_CMD="apt-get"
CONFIG_PATH=$HOME/.config/polycube/
SWAGGER_CODEGEN_CONFIG_FILENAME=swagger_codegen_config.json

if hash apt 2>/dev/null; then
    APT_CMD="apt"
fi

sudo $APT_CMD update -y

sudo $APT_CMD install git -y

# Install python dependencies
sudo $APT_CMD install python-minimal -y
sudo $APT_CMD install python-pip -y
sudo $APT_CMD install python-setuptools -y

# Install java for swagger-codegense
sudo $APT_CMD install default-jre -yh
sudo $APT_CMD install default-jdk -y

# Install Apache Maven
sudo $APT_CMD install maven -y

#git clone https://github.com/polycube-network/pyang-swagger.git -b polycube
if cd pyang-swagger; then
	git fetch origin
	git checkout polycube
	git pull origin polycube
else
	git clone https://github.com/polycube-network/pyang-swagger.git -b polycube
	cd pyang-swagger
fi

./install.sh

cd $HOME/dev
if cd swagger-codegen; then
	git fetch origin
	git checkout polycube
	git pull origin polycube
else
	git clone https://github.com/polycube-network/swagger-codegen.git -b polycube
	cd swagger-codegen
fi

GIT_SWAGGER_CODEGEN_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
GIT_SWAGGER_CODEGEN_COMMIT_HASH="$(git log -1 --format=%h)"

mkdir -p $CONFIG_PATH

mvn -T $(getconf _NPROCESSORS_ONLN) clean package -DskipTests

cp modules/swagger-codegen-cli/target/swagger-codegen-cli.jar $POLYCUBE_HOME

sudo cp $POLYCUBE_HOME/scripts/polycube-codegen.sh /usr/local/bin/polycube-codegen

#Create configuration file for swagger-codegen
cat > ${CONFIG_PATH}${SWAGGER_CODEGEN_CONFIG_FILENAME} << EOF
{
  "gitUserId" : "polycube-network",
  "gitRepoId" : "${GIT_SWAGGER_CODEGEN_BRANCH}/${GIT_SWAGGER_CODEGEN_COMMIT_HASH}"
}
EOF

success_message

cd $_pwd
