#!/bin/sh

set -e
set -x

autoreconf -ivf
./configure
