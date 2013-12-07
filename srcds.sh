#!/bin/bash

# Ensure that the current working directory is the top level of the game folder tree
cd "`dirname \"$0\"`"

# Pass arguments to the real srcds binary
exec SrcDS.app/Contents/MacOS/SrcDS "$@"
