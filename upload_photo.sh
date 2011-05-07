#!/bin/sh

PATH=$PATH:~/release/clickr/
QUEUE_DIR=~/.clickr-queue

PHOTO=`ls -t $QUEUE_DIR/*.jpg | tail -n1`

# If there is no photo, bail.
if [ ! $PHOTO ]
then
	echo "No photos to upload in $QUEUE_DIR"
	exit
fi

# If there is no description file, bail.
DESCR_FILE=$PHOTO.description
if [ ! -e $DESCR_FILE ]
then
	echo $DESCR_FILE
	exit
fi

TITLE=`egrep "^TITLE	" $DESCR_FILE | awk 'BEGIN {FS="	"} {print $2}'`
DESCRIPTION=`egrep "^DESCRIPTION	" $DESCR_FILE | awk 'BEGIN {FS="	"} {print $2}'`
TAGS=`egrep "^TAGS	" $DESCR_FILE | awk 'BEGIN {FS="	"} {print $2}'`

echo $PHOTO
echo $TITLE
echo $DESCRIPTION
echo $TAGS

clickr -f "$PHOTO" -t "$TITLE" -d "$DESCRIPTION" -l "$TAGS"

rm $PHOTO $DESCR_FILE
