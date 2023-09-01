#!/bin/sh

gcc patchtool.c -o patchtool || exit
./patchtool > .lastpatch.txt || exit
cat .lastpatch.txt patchnotes.txt >  .patchnotes.txt || exit
mv .patchnotes.txt patchnotes.txt

