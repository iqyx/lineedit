LineEdit - a basic command line editor
===========================================

Lineedit aims to be a lightweight command line editor usable over common stream
interfaces (like sockets, serial ports, etc.). It was written to be used on an
embedded platform and its features and code are optimized for embedded environments.


Features
--------------------------------------

Version 0.1.0 implemented features:

* ANSI escape sequence support (basic input parsing and output formatting)
* single-line editing capabilities (cursor keys, backspace, del)
* preparation for more complex shell implementations with autocompletion
* customizable command prompt
* history saving and recall

TODO:

* API cleanup
* get rid of string manipulation functions


Library status
--------------------------------------

Library is currently usable for basic editing. it is still work in progress with
features and fixes continuously added.


Usage
--------------------------------------

See examples/example1.c file. Terminal needs to be in non-canonical mode for this
example to work correctly. You can use example1.sh script to set it using stty
utility.
