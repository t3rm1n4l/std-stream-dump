std-stream-capture
==================

A standalone tool for capturing stdin and stdout streams

### Usage
    $ make
    $ cp out /usr/local/bin/target_binary_name

    # Execute for capture
    $ export BIN_PATH=/home/slynux/target_binaries
    $ echo stdin_text | /usr/local/bin/target_binary_name

    # View captured data files
    $ ls /tmp/target_binary_name*

Copy std-stream-capture's binary `out` to the target binary location. Copy
actual target binary to a auxilary location and set environment variable
BIN_PATH to auxilary directory location. Execute the proxy binary program.
You can find the data dump for stdin, stdout, stderr and arguments at
a location /tmp/program_name.random directory.
