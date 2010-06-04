#!/bin/bash

function make_format {
  cat $1 | awk '{print "%(" $_ ")s"}' | xargs echo
}

[ ! -x ./logging_test ] && make logging_test

LOGGING_LOG_FORMAT=`make_format logging_variables.txt` ./logging_test
