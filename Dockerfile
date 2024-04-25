# Use an official Ubuntu base image that supports multiple architectures
FROM alpine:latest as builder

# Set the working directory
WORKDIR /app

# Install GCC for compiling C programs
RUN apk add --no-cache gcc musl-dev
# Copy the C source file into the Docker image
COPY main.c /app

# Compile the C program to a binary named 'program'
RUN gcc -static -D DOCKER=true main.c -o wol

FROM alpine:latest
ENV MAC_ADDRESS="a8:a1:59:e8:b3:16"
ENV LISTEN_PORT="5431"
COPY --from=builder /app/wol /app/

expose $LISTEN_PORT

# Command to run the compiled binary
CMD ["sh", "-c", "/app/wol $MAC_ADDRESS $LISTEN_PORT"]
