LineEdit - a basic command line editor
===========================================

Lineedit aims to be a lightweight command line editor usable over common stream
interfaces (like sockets, serial ports, etc.). It was written to be used on an
embedded platform and its features and code are optimized for embedded environments.


Features
--------------------------------------

Main implemented features are:

* ANSI escape sequence support (basic input parsing and output formatting)
* single-line editing capabilities (cursor keys, backspace, del)
* preparation for more complex shell implementations with autocompletion
* customizable command prompt

TODO:

* support for history recall
* API cleanup


Library status
--------------------------------------

Library is currently usable for basic editing. it is still work in progress with
features and fixes continuously added.


Usage
--------------------------------------

See examples/example1.c file. Terminal needs to be in non-canonical mode for this
example to work correctly. You can use example1.sh script to set it using stty
utility.
