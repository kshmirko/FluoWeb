#!/bin/bash
./ltr210-fluo 2
if [ $? -eq 0 ]; then
	zip ltr210.zip ltr210_*.sav
	rm ltr210_*.sav
fi; 