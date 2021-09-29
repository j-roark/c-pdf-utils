#!/bin/bash

gcc -o ./main ./main.c `pkg-config --cflags --libs cairo poppler-glib` -I /usr/include/ -I /usr/include/glib-2.0/ -I /usr/include/cairo/ -L /usr/local/lib/ -ljson-c

./main
