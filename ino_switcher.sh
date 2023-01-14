#!/bin/sh

SWITCHED_FILE=$1

rm ./simpleble.ino

if [[ "$SWITCHED_FILE" == "simpleble" || "$SWITCHED_FILE" == "user_terminal" || "$SWITCHED_FILE" == "bl_passthrough" ]] ; then
    echo "In examples"
    ln ./examples/$SWITCHED_FILE.ino ./simpleble.ino
fi

if [[ $SWITCHED_FILE == "consistency" || $SWITCHED_FILE == "misc_tests" || $SWITCHED_FILE == "speedtester" ]] ; then
    echo "In tests"
    ln ./tests/$SWITCHED_FILE.ino ./simpleble.ino
fi

