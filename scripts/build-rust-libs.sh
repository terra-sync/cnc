#!/bin/bash
set -e

for dir in rust/*; do
    (cd "$dir" && cargo build)
done
