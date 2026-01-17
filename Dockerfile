FROM ubuntu:20.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    wget \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Download and Install SystemC 2.3.3
WORKDIR /tmp
RUN wget https://github.com/accellera-official/systemc/archive/refs/tags/2.3.3.tar.gz -O systemc-2.3.3.tar.gz \
    && mkdir systemc-src \
    && tar -xzf systemc-2.3.3.tar.gz -C systemc-src --strip-components=1 \
    && cd systemc-src \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/systemc -DCMAKE_CXX_STANDARD=14 \
    && make -j$(nproc) \
    && make install \
    && cd /tmp \
    && rm -rf systemc-src systemc-2.3.3.tar.gz

# Set environment variables
ENV SYSTEMC_HOME=/usr/local/systemc
ENV LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$SYSTEMC_HOME/lib-linux64:$LD_LIBRARY_PATH

# Work directory for the project
WORKDIR /app

# Copy project files
COPY . /app

# Build the project (if Makefile exists)
CMD ["make", "run"]
