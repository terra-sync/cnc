#! /bin/bash

if docker ps --format "{{.Names}}" | grep -q "cnc-mongodb-origin"; then
    docker stop cnc-mongodb-origin
    docker rm cnc-mongodb-origin
    echo "MongoDB Docker container was successfully removed."
else
    echo "MongoDB Docker container is not currently running."
fi

if docker ps --format "{{.Names}}" | grep -q "cnc-mongodb-target"; then
    docker stop cnc-mongodb-target
    docker rm cnc-mongodb-target
    echo "MongoDB Docker container was successfully removed."
else
    echo "MongoDB Docker container is not currently running."
fi

