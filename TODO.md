
DONE: Check if currently is paused, and if it do something I forgot why

DONE: If we click on the bar to jump we sohuld reset the segments variables

DONE: Check if we currently clicking

DONE: check if merge segments is actually correct

DONE: Checkout Raygui

TODO: Each Gui component has a bool group for interactivity
TODO: make a Gui component for mpv

TODO: Get first frame to be the thumb nail with option to save another image as thumb nail
TODO: SQLite almagamation to store the data

TODO: Fix Scrolling with small menus to the listview
TODO: Make wheel speed change with stuff
DONE: Add SCROLL_SLIDER_SIZE to the listview
TODO: Add new option  SCROLL_SLIDER_WIDTH to the listview
TODO: Fix Antialiasing Fxaa

TODO: Add "add to favs" in menu
TODO: Create List of Favs
TODO: Create Copy path or open in explorer
TODO: Resize on ui


Is this useful?
#!/usr/bin/sh
make clean;make VERBOSE=y all &> /tmp/make_output.txt
compiledb --parse /tmp/make_output.txt

TODO: create variadic arguments
TODO: Fuzzy Match search files in the list view
TODO: list view zoom in
TODO: Show files
TODO: MPV has a file that triggers an assert, investigate what it is Assertion failed: x1 <= img->w && y1 <= img->h, file ../video/mp_image.c, line 564 [ERROR] command exited with exit code 3

TODO: Make SQLite database for watched segments and shit
TODO: Segmnets are scuffed on big files idk why

TODO: Main Page for last watched with indication of how much you have watched, Netflix style ok ?
