#!/bin/sh

QUEUE_DIR=sdstrowes@sdstrowes.co.uk:~/.clickr-queue
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
if [ -e "$QUEUE_DIR" ]  && [ ! -d "$QUEUE_DIR" ]
then
    echo "Something called ~/.clickr-queue exists! Remove please."
	exit 1
elif [ ! -e "$QUEUE_DIR" ]
then
	mkdir -p "$QUEUE_DIR"
fi

echo $PHOTO
echo $TITLE
echo $DESCRIPTION
echo $TAGS

$cp $PHOTO $QUEUE_DIR
echo "TITLE	$TITLE" > $QUEUE_DIR/$PHOTO.description
echo "DESCRIPTION	$DESCRIPTION" >> $QUEUE_DIR/$PHOTO.description
echo "TAGS	$TAGS" >> $QUEUE_DIR/$PHOTO.description
