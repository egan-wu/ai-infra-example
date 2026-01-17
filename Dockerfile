# Dockerfile for Dual-Mode SystemC + Gem5 Simulation

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install Dependencies
# Gem5 dependencies + SystemC dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    m4 \
    scons \
    zlib1g \
    zlib1g-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libprotoc-dev \
    libgoogle-perftools-dev \
    python3-dev \
    python3-six \
    python-is-python3 \
    libboost-all-dev \
    pkg-config \
    wget \
    cmake \
    gcc-arm-linux-gnueabihf \
    && rm -rf /var/lib/apt/lists/*

# 2. Download and Install SystemC 2.3.3
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

# 3. Build Gem5 (v23.1) as a Shared Library
# This is the time-consuming step.
WORKDIR /opt
RUN git clone --branch v23.1 https://gem5.googlesource.com/public/gem5
WORKDIR /opt/gem5

# We build the optimized version of the shared library for ARM.
# --without-tcmalloc avoids potential conflicts with SystemC or other libs in some envs,
# but tcmalloc is standard for Gem5. We keep it.
# 'build/ARM/libgem5_opt.so' is the target.
RUN scons build/ARM/libgem5_opt.so -j$(nproc) --without-tcmalloc

# Set Environment Variables
ENV SYSTEMC_HOME=/usr/local/systemc
ENV GEM5_HOME=/opt/gem5
# Add Gem5 lib to LD_LIBRARY_PATH
ENV LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$SYSTEMC_HOME/lib-linux64:$GEM5_HOME/build/ARM:$LD_LIBRARY_PATH
# Add Gem5 python paths
ENV PYTHONPATH=$GEM5_HOME/src/python:$GEM5_HOME/build/ARM/python:$PYTHONPATH

# 4. Project Setup
WORKDIR /app
COPY . /app

# Compile the Guest Binary for Gem5
RUN arm-linux-gnueabihf-gcc -static -o tpu_test_binary test_app.c

# Default command: build simple and run
CMD ["make", "simple", "&&", "./simulation"]
