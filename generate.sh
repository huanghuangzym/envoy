#!/bin/bash

set -ex

bazel_dir="bazel-${PWD##*/}"

find .  -name '*.cc' > sourcefiles.txt
