#! /bin/bash

if docker ps --format "{{.Names}}" | grep -q "cnc-postgres"; then
    docker stop cnc-postgres
    docker rm cnc-postgres
    echo "PostgreSQL Docker container was successfully removed."
else
    echo "PostgreSQL Docker container is not currently running."
fi
