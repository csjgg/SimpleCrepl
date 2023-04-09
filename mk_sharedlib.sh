#!/bin/bash

gcc -shared -fpic -Wno-implicit-function-declaration -o sharedlib.so ./tem/*
