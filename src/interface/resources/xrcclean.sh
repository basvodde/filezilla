#! /bin/sh

# XRCed has the nasty habit to add empty lines to the xrc files. This small script removes them.

if [ -z $1 ]; then
  # No argument given, find all .xrc files and call ourself once for each .xrc file
  find . -name "*.xrc" | xargs ./xrcclean.sh
else
  # Remove newlines and lines containing only spaces from all given files
  while test $# != 0; do
    cat $1 | sed -e "s/^ *$//" | sed -e "/^$/d" > $1_new
    mv $1_new $1
    shift
  done
fi

