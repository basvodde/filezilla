#! /bin/sh

# Script to clean .png files using pngcrush
# Should be run after adding of modifying .png files

# Settings
PNGCRUSH=pngcrush
PNGCRUSH_OPTIONS="-brute -reduce -rem gAMA -rem tIME -rem pHYs -rem cHRM -rem bKGD"

FILES=`find -name "*.png"`

for i in $FILES; do
  $PNGCRUSH $PNGCRUSH_OPTIONS $i $i~ | grep -v 'IDAT length with method' | grep -v '^ |' | grep -v '^$' | grep -v 'CPU time used' | grep -v 'seconds)' | grep -v ' version: ' | grep -v 'versions are different' || die
  mv $i~ $i
done
