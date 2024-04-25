# Use an official Ubuntu base image that supports multiple architectures
FROM ubuntu:latest

# Set the working directory
WORKDIR /app

# Install GCC for compiling C programs
RUN apt-get update && apt-get install -y gcc

# Copy the C source file into the Docker image
COPY main.c /app

# Compile the C program to a binary named 'program'
RUN gcc -D DOCKER=true main.c -o wol

ENV MAC_ADDRESS="a8:a1:59:e8:b3:16"
ENV LISTEN_PORT="5431"

expose $LISTEN_PORT



# Command to run the compiled binary
CMD ["bash", "-c", "./wol $MAC_ADDRESS $LISTEN_PORT"]
