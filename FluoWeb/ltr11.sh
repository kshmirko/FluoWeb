#!/bin/bash
./ltr11-fluo 3

if [ $? -eq 0 ]; then
	zip ltr11.zip ltr11_*.sav
	rm ltr11_*.sav
fi
