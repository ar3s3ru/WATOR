#!/bin/bash

rm -rf *.o tmp

# Creating WATOR library
if ! gcc -Wall -Werror -pedantic -g -c wator.c ; then
	echo "Error creating wator.c" 1>&2
	exit 1
fi

if ! ar rcs libWator.a wator.o ; then
	echo "Error creating libWator.a" 1>&2
	exit 1
fi

mv *.a ../lib

# Creating Utils library
if ! gcc -Wall -Werror -pedantic -g -c utils.c ; then
	echo "Error creating utils.o" 1>&2
	exit 1
fi

if ! ar rcs libUtils.a utils.o ; then
	echo "Error creating libUtils.a" 1>&2
	exit 1
fi

# Creating wator process
if ! gcc -Wall -Werror -pedantic -g -c wator_process.c ; then
	echo "Error building wator_process.o"
	exit 1
fi

if ! gcc -o wator wator_process.o -L ../lib -pthread -lWator -lUtils ; then
	echo "Error compiling wator"
	exit 1
fi

# Creating visualizer process
if ! gcc -Wall -Werror -pedantic -g -c visualizer.c ; then
        echo "Error building visualizer.o"
        exit 1
fi

if ! gcc -o visualizer visualizer.o -L ../lib -lUtils -pthread ; then
	echo "Error compiling visualizer"
	exit 1
fi

rm  *.o

exit 0
