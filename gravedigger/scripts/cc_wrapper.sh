#!/bin/bash

# Remove any OpenSSL include paths from cosmocc
args=()
skip_next=false
for arg in "$@"; do
    if $skip_next; then
        skip_next=false
        continue
    fi
    
    if [[ $arg == *"openssl"* ]]; then
        if [[ $arg == "-I"* ]]; then
            continue
        elif [[ $arg == "-L"* ]]; then
            continue
        elif [[ $arg == "-I" || $arg == "-L" ]]; then
            skip_next=true
            continue
        fi
    fi
    args+=("$arg")
done

# Call cosmocc with filtered args
/home/forge/tools.undernet.work/ape-playground/cosmocc/bin/cosmocc "${args[@]}"
