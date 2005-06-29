#! /bin/sh

# Script to clean .png files using pngcrush
# Should be run after adding or modifying .png files

# Settings
PNGCRUSH=pngcrush
PNGCRUSH_OPTIONS="-brute -reduce -rem gAMA -rem tIME -rem pHYs -rem cHRM -rem bKGD -rem ICCP -rem iTXt -rem tEXt -rem zTXt -rem sBIT -rem sPLT -rem hIST"
PNGCRUSH_WNDSIZES="32 16 8 4 2 1 512"

FILES=$@

if [ -z "$FILES" ]; then
  FILES=`find -name "*.png"`
fi

for i in $FILES; do
  if [ -f $i ]; then
    for j in $PNGCRUSH_WNDSIZES; do
      echo -n "."
      $PNGCRUSH -w $j $PNGCRUSH_OPTIONS $i $i~ | grep -v 'IDAT length with method' | grep -v '^ |' | grep -v '^$' | grep -v 'CPU time used' | grep -v 'seconds)' | grep -v ' version: ' | grep -v 'versions are different' > pngcrush.log || die
      if ! cat pngcrush.log | grep '\(no change\)' > /dev/null; then
        cat pngcrush.log
        mv $i~ $i
      else
        rm $i~
      fi
    done
  else
    echo "Warning: $i not a regular file"
  fi
done

rm -f pngcrush.log
