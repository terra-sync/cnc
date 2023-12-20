#! /bin/bash

if [ -x "$(command -v docker)" ]; then
    echo "Docker installed"

else
    echo "You need to install Docker and try again.
To install docker visit (https://docs.docker.com/engine/install/)."
    exit 1
fi

docker_args_origin="--name cnc-mongodb-origin -d -p 27017:27017 \
			 -e MONGO_INITDB_ROOT_USERNAME=root \
			 -e MONGO_INITDB_ROOT_PASSWORD=root \
			 mongodb/mongodb-community-server:latest"

docker_args_target="--name cnc-mongodb-target -d -p 27018:27017 \
			 -e MONGO_INITDB_ROOT_USERNAME=root \
			 -e MONGO_INITDB_ROOT_PASSWORD=root \
			 mongodb/mongodb-community-server:latest"

if docker run $docker_args_origin; then
    echo "Mongodb host container was successfully set up."
else
    echo "Failed to start the Mongodb host container."
    exit 1
fi

if docker run $docker_args_target; then
    echo "Mongodb target container was successfully set up."
else
    echo "Failed to start the Mongo target container."
    exit 1
fi
