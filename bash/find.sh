#! /bin/bash
# find utility for OS course, Vladimir Mironovich

function RecurseDirs
{
	oldIFS=$IFS
	IFS=$' '
	for f in "$@"
	do
		if [ -`echo "$TYPE"` "$f" ]
		then
			result=`echo "$f" | egrep -i "$INAME"`
			if [ "$result" ]
			then
				echo $result
			fi
		fi
		if [[ -d "$f" ]]; then
			RecurseDirs `echo $f/*`
		fi
	done
	IFS=$oldIFS
}
	
USAGE="$0 dirname -type -iname"
DIR=""
TYPE=""
INAME=""

while [ ! -z "$1" ]
do
	case "$1" in
		"-iname")
		shift
		INAME="$1"
		;;
		"-type")
		shift
		TYPE="$1"
		;;
		*)
		if [ -z "$DIR" ]
		then
			DIR="$1"
		else
			echo $USAGE
			exit -1;
		fi
		;;
	esac
	shift
done
		
if [ -z "$DIR" ]||[ -z "$TYPE" ]||[ -z "$INAME" ]
then
	echo $USAGE
	exit -1;
fi

RecurseDirs "$DIR"
