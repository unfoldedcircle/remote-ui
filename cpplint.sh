#! /bin/bash
# "-whitespace" is required to get rid off the annoying
# `At least two spaces is best between code and comments  [whitespace/comments] [2]`
# false-positives in some copyright file headers.
cpplint --filter=-build/include_subdir,-build/c++11,-whitespace --linelength=120 --root=src --recursive src
