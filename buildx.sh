#!/usr/bin/env sh


ACTION=$1

if [ "$ACTION" = "setup" ]; then
    docker buildx create --name wol-builder --use --driver docker-container
    docker buildx use wol-builder
    docker buildx inspect --bootstrap
    exit 0
fi

if [ "$ACTION" = "build" ]; then
    docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 -t wol:latest .
    exit 0
fi

if [ "$ACTION" = "push" ]; then
    printf "Enter your dockerhub username: "
    read DOCKER_USERNAME
    docker login -u $DOCKER_USERNAME

    docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 -t $DOCKER_USERNAME/wol:latest --push .
    exit 0
fi

echo "Usage:" $0 "[setup|build|push]"
    cat <<EOF
            setup: setup docker buildx
            build: build docker image
            push: build and push docker image to dockerhub
EOF
