FROM armforge

# Install development tools and utilities
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    # Build tools
    build-essential \
    cmake \
    make \
    pkg-config \
    # Development tools
    git \
    vim \
    nano \
    curl \
    wget \
    # Code quality tools
    clang-format \
    clangd \
    cpplint \
    # Python for development
    python3-pip \
    python3-colcon-common-extensions \
    # Additional utilities
    htop \
    tree \
    # VS Code server dependencies
    ca-certificates \
    procps \
    unzip \
    tar \
    && rm -rf /var/lib/apt/lists/*

# Install additional Python packages for development
RUN pip3 install --no-cache-dir \
    flake8 \
    black \
    pytest

# Set up ROS2 environment
RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc

# Create workspace directory
WORKDIR /ws

# Set default command
CMD ["/bin/bash"]
