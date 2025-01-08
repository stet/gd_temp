#!/bin/sh
COSMOCC=/home/forge/tools.undernet.work/ape-playground/cosmocc/bin/cosmocc
exec $COSMOCC "$@" 2> >(grep -v "superconfigure/o/lib/openssl/include" >&2)
