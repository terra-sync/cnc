#! /bin/bash

if [ -x "$(command -v docker)" ]; then
    echo "Docker installed"

else
    echo "You need to install Docker and try again.
To install docker visit (https://docs.docker.com/engine/install/)."
    exit 1
fi

docker_args_origin="--name cnc-postgres-origin -d \
		  -e POSTGRES_PASSWORD=password   \
                  -e POSTGRES_DB=postgres_origin \
		  -p 5432:5432 postgres:14"

if docker run $docker_args_origin; then
    echo "Postgres-origin container was successfully set up."
else
    echo "Failed to start the Postgres-origin container."
    exit 1
fi

docker_args_target="--name cnc-postgres-target -d \
                    -e POSTGRES_PASSWORD=password \
                    -e POSTGRES_DB=postgres_target \
                    -p 5433:5432 postgres:14"

if docker run $docker_args_target; then
    echo "Postgres-target container was successfully set up."
else
    echo "Failed to start the Postgres-target container."
    exit 1
fi
