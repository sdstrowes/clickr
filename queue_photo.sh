#!/bin/sh

host=sdstrowes@sdstrowes.co.uk
queue_dir=.clickr-queue
cp=scp

printHelp()
{
	echo "Usage: $0 -p /path/to/photo [-t title] [-d description]"
	exit
}

while getopts "p:d:t:l:" opt; do
    case $opt in
		p)
			PHOTO="$OPTARG"
			;;
		d)
			DESCRIPTION="$OPTARG"
			;;
		t)
			TITLE="$OPTARG"
			;;
		l)
			TAGS="$OPTARG"
			;;
		\?)
			printHelp
			exit 1
			;;
	esac
done

if [ ! "$PHOTO" ]
then
	printHelp
	exit 1
fi

if [ ! "$DESCRIPTION" ]
then
	DESCRIPTION=`basename $PHOTO`
fi

if [ ! "$TITLE" ]
then
	TITLE=`basename $PHOTO`
fi



# Ensure there's a valid upload queue directory.
ssh $host "if [ -e \"$queue_dir\" ]  && [ ! -d \"$queue_dir\" ] ; then
    echo 'Something called '$queue_dir' exists! Remove please.'
	exit 1
elif [ ! -e \"$queue_dir\" ]
then
	mkdir -p \"$queue_dir\"
fi"

echo $PHOTO
echo $TITLE
echo $DESCRIPTION
echo $TAGS

$cp $PHOTO $host:$queue_dir

tmp=/tmp/$PHOTO.description

echo "TITLE	$TITLE"               >  $tmp 
echo "DESCRIPTION	$DESCRIPTION" >> $tmp
echo "TAGS	$TAGS"                >> $tmp

$cp $tmp $host:$queue_dir
