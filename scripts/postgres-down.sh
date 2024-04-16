#! /bin/bash

if docker ps --format "{{.Names}}" | grep -q "cnc-postgres-origin"; then
    docker stop cnc-postgres-origin
    docker rm cnc-postgres-origin
    echo "Postgres-origin docker container was successfully removed."
else
    echo "Postgre-origin docker container is not currently running."
fi

if docker ps --format "{{.Names}}" | grep -q "cnc-postgres-target"; then
    docker stop cnc-postgres-target
    docker rm cnc-postgres-target
    echo "Postgres-target docker container was successfully removed."
else
    echo "Postgres-target docker container is not currently running."
fi
