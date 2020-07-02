#!/bin/bash

# This script is used to create an archive with all the polycube sources, including submodules. 
# Currently github can create automatically the source pack, but it fails to include 
# all the files belonging to submodules, hence making this function unsuitable for this project.
# This script allows to download either a specific branch or a tagged version of Polycube, 
# and create both a ZIP and a TAR containing all the sources.
# Put this script where you want to clone Polycube.


DIR=`pwd`

echo "example of zip name format: polycube-0.9.0-rc"

git clone https://github.com/polycube-network/polycube.git
cd polycube
git submodule update --init --recursive

# give the possibility to create an archive of a certain branch or tag
echo "----------------------------------------------------------------"
echo -n "digit b for branch or t for tag: "
read choice

if [ $choice == 'b' ]
then	
	echo "----------------------------------------------------------------"
	echo "current branches"
	git branch -a
	
	echo -n "digit the branch or press enter for master: "
	read branch

	if [ -z "$branch" ] || [ $branch == 'master' ]
	then 
		echo -n "digit the version: "
		read version
		echo "compressing Polycube master with version equal to $version ..." 
	else
		if [ `git branch -a | egrep "^[[:space:]]+${branch}$"` ]
		then
		    echo -n "digit the version: "
		    read version
		    git checkout $branch
		    echo "compressing Polycube $branch with version equal to $version ..." 
		else
			echo "branch $branch does not exist"
			exit 0
		fi		
	fi
elif [ $choice == 't' ] 
then
	echo "----------------------------------------------------------------"
	echo "current tags"
	git tag

	echo -n "digit the tag: "
	read tag
	if [ `git tag | egrep "${tag}$"` ]
	then
		git checkout $tag
		echo "compressing Polycube with version equal to $tag ..."
		tag=${tag/v}
		tag=${tag/-rc}
		version=$tag
	else
		echo "tag $tag does not exist"
		exit 0
	fi
else
	echo "error"
	exit 0
fi

cd $DIR

# rename folder
mv polycube polycube-"$version"-rc

# create zip
ZIP=polycube-"$version"-rc.zip

zip -rq $ZIP polycube-"$version"-rc -x '*.git*'  

if test -f "$ZIP"
then
	echo "$ZIP created"
else 
	echo "error: $ZIP not created"
fi

# create tar
TAR=polycube-"$version"-rc.tar.gz

tar --exclude .git -czf $TAR polycube-"$version"-rc

if test -f "$TAR"
then
	echo "$TAR created"
else 
	echo "error: $TAR not created"
fi
