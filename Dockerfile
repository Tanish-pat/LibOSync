# Use Ubuntu base image
FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    build-essential \
    net-tools \
    iputils-ping \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app

# Copy source files
COPY . .

# Compile the code
RUN make all && chmod +x server client

# Expose server port
EXPOSE 6004

# Default command: run the server
CMD ["./server"]
