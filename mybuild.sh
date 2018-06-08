#!/bin/bash
function backfile() {
	filename=${1:0-1}
	if [ "$filename" == "*" ]; then
		return
	fi
	filename=$1
	file1=${filename#*/}
	file=${file1#*/}
	dir_l1=${file%%/*}
	#echo "$file --- $dir_l1"
	if [ "$dir_l1" != "*" ]; then
		if [ ! -d "$dir_l1" ]; then 
			echo "the '$dir_l1' must is a directory and exist"
			rm -rf wtwd-m/back_temp
			exit 1
		fi

		if [ -f "$file" ]; then
		    cp -af "$file" ./wtwd-m/back_temp/"$file"
		else
			rm -rf "./wtwd-m/back_temp/$file"
			echo "$file" >> wtwd-m/back_temp/del_file_list
		fi
	fi
}

function allFiles() {
	for file in $1/*
	do
		if [ -d "$file" ]; then
			basedir=${file#*/}
			basedir=${basedir#*/}
			#echo "directory: $basedir"
			if [ ! -d "$basedir" ]; then
				echo "$basedir" >> wtwd-m/back_temp/del_file_list
				rm -rf "./wtwd-m/back_temp/$basedir"
			else
				allFiles "$file"
			fi
		else
			backfile "$file"
		fi
	done
}

function backallfiles() {
	if [ ! -d "wtwd-m/back_temp" ]; then
		mkdir wtwd-m/back_temp
		cp -af wtwd-m/$1/* wtwd-m/back_temp/

		touch wtwd-m/back_temp/build-project wtwd-m/back_temp/del_file_list
		echo "$1 $2" > wtwd-m/back_temp/build-project

		allFiles wtwd-m/$1
	fi
}

function storefiles() {
	if [ -d "wtwd-m/back_temp" ]; then
		cp -af wtwd-m/back_temp/* ./

		while read line; do
			if [ -f "$line" ] || [ -d "$line" ]; then
				rm -r "$line"
			fi
		done < wtwd-m/back_temp/del_file_list

		rm build-project
		rm del_file_list
		echo "restore file is finished"
	else
		echo "the wtwd-m/back_temp directory is not exist"
	fi
}

function copyoverfiles() {
	if [ ! -d "wtwd-m/$1" ]; then
		echo "this directory 'wtwd-m/$1' is not exist"
		exit 1
	fi
	cp -af wtwd-m/back_temp/* ./
	rm -rf wtwd-m/back_temp
	backallfiles $1 $2
	cp -af wtwd-m/$1/* ./
}

if [ $# -lt 2 ]; then
	if [ $# -gt 0 ]; then
		if [ $1 = restore ]; then
			storefiles
			rm -rf wtwd-m/back_temp
			exit 0
		fi

		if [ $1 = copyover ]; then
			if [ ! -d "wtwd-m/back_temp" ]; then
				echo "please first ./mybuild.sh product project"
				exit 1
			fi
			buildprj=`cat wtwd-m/back_temp/build-project`
			prj=${buildprj% *}
			typ=${buildprj#* }
			copyoverfiles $prj $typ
			echo "copy 'wtwd-m/$prj' is finished"
			exit 0
		fi
	fi

    echo -e "./mybuild.sh product project -- eg: ./mybuild.sh m166 mac_atcmd\n./mybuild.sh restore\n./mybuild.sh copyover"
    exit 1
fi

if [ ! -d "wtwd-m/$1" ]; then
	echo "this directory 'wtwd-m/$1' is not exist"
	exit 1
fi

if [ ! -d "projects/$2" ]; then
	echo "this directory 'projects/$2' is not exist"
	exit 1
fi

backallfiles $1 $2
cp -af wtwd-m/$1/* ./

make distclean
make setup p=$2
make
storefiles
