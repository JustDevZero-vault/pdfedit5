#!/bin/bash

PROG_NAME=`basename $0`

usage()
{
	echo "$PROG_NAME usage:"
	echo -e "\t$PROG_NAME patch_file [-dry] [-revert] files*"
	echo -e "\n\tpatch_file\tpatch to apply."
	echo -e "\tfiles\tfiles to be used"
	echo -e "\t-dry\tdry run - don't do any changes just try what would be done."
	echo -e "\t-revert\trevers given patch"
	exit 1
}

if [ $# -lt 1 ]
then
       usage
fi	

PATCH_FILE=$1
FILES=
DRY=
REVERT=

shift
for param in $*
do
	if [ "i$1" = "i-dry" ]
	then
		DRY=--dry-run
		shift
		continue
	fi

	if [ "i$1" = "i-revert" ]
	then
		REVERT=-R
		shift
		continue
	fi

	if [ -f $1 ]
	then
		FILES="$FILES $1"
		shift
		continue
	fi

	echo "Unrecognized parameter $1 (skipping)"
	shift
done

echo patch to be used: $PATCH_FILE
echo files to be patched: $FILES
[ -n "$DRY" ] && echo dry-run
[ -n "$REVERT" ] && echo reverting

for FILE in $FILES
do
	echo -n "Patching $FILE "
#	cat $PATCH_FILE | sed -e "s/\(^---\)\s\(.*\)\s.*/\1 ${FILE}/" -e "s/\(^+++\)\s\(.*\)\s.*/\1 ${FILE}/" | patch -p0 --dry-run && echo ok || echo failed

	patch <$PATCH_FILE -p0 $DRY $REVERT $FILE >& /dev/null && echo -e "\tok" || echo -e "\tfailed"
done