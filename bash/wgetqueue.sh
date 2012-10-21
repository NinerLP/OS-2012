#!/bin/bash

shopt -s nullglob

PID_FILE="wgetter.pid" #parent pid
LOCK_FILE="wgetter.lock" #daemon is up
GET_LOCK_FILE="wgetter_get.lock" #working on something
QPATH=$HOME/.nyaqueue #default queue, missed class, not sure what's wrong with it

mkdir -p "$QPATH"
mkdir -p "$QPATH/requests"

function redirect-std() { 
    #lots of ways to try and shut up damn scripts, although probably this thing is not needed at all
    [[ -t 0 ]] && exec </dev/null
    [[ -t 1 ]] && exec >/dev/null
    [[ -t 2 ]] && exec 2>/dev/null
}

function stop()
{
    #stopping daemon
	if [ -e ${PID_FILE} ]
	then
		_pid=$(cat ${PID_FILE})
		kill $_pid
		rt=$?
		if [ "$rt" == "0" ]
		then
			echo "Wgetter daemon stopped"
		else
			echo "Error stopping daemon"
		fi
	else
		echo "Daemon not running"
	fi
}

function get() {
    #getting with wget
	( 
	redirect-std
	trap "{ rm -f ${GET_LOCK_FILE}; exit 255; }" TERM INT EXIT
	if [ ! -e ${GET_LOCK_FILE} ]
    then
		echo "$$" > ${GET_LOCK_FILE}
		for a in "$QPATH/requests"/*
        do
            url=`cat "$a"`
            rm "$a"
            wget -qP "$url" "$url"
        done
	fi

	exit 0 
	)&
}

function start()
{
	#adding adresses to queue
	for a in "$@"
	do
		r=`mktemp --tmpdir="$QPATH/requests"`
		echo "$a" > "$r"
	done
	
    #if daemon working, do nothing
	if [ -e $PID_FILE ]
	then
		_pid=$(cat ${PID_FILE})
		if [ -e /proc/${_pid} ]
		then
			#echo "Daemon already running with pid = $_pid"
			exit 0
		fi
	fi
    
    #or start daemon
	redirect-std
	(
		
		trap "{ rm -f ${PID_FILE}; exit 255; }" TERM INT EXIT
		
		while [ 1 ]
		do
			if [ ! -e ${GET_LOCK_FILE} ]
			then
				get
			fi
			sleep 1
		done
		exit 0
	)&
	
	echo $! > ${PID_FILE}
}

#either ./wgetqueue.sh ~ ~ ~ adresses or ./wgetqueue.sh stop
case $1 in
	"stop")
		stop
		;;
	*)
		start "$@"
esac
exit

