#!/bin/sh

for i in build/bin/*
do
    echo "Copying $i"
    cat $i | novaterm put file:///usr/bin/$(basename $i)
done

echo "Copying libpurple"
cat build/libpurple.so | novaterm put file:///usr/lib/libpurple.so
