
DONE: Check if currently is paused, and if it do something I forgot why

DONE: If we click on the bar to jump we sohuld reset the segments variables

DONE: Check if we currently clicking

check if merge segments is actually correct

TODO: Checkout Raygui

TODO: Get first frame to be the thumb nail with option to save another image as thumb nail


Is this useful?
#!/usr/bin/sh
make clean;make VERBOSE=y all &> /tmp/make_output.txt
compiledb --parse /tmp/make_output.txt
