#! /bin/bash

if [ -x "$(command -v docker)" ]; then
    echo "Docker installed"

else
    echo "You need to install Docker and try again.
To install docker visit (https://docs.docker.com/engine/install/)."
    exit 1
fi

docker_args="--name cnc-postgres -d \
		  -e POSTGRES_PASSWORD=password \
		  -p 5432:5432 postgres:15"

if docker run $docker_args; then
    echo "Postgres container was successfully set up."
else
    echo "Failed to start the Postgres container."
    exit 1
fi
