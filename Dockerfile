FROM ros:humble

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    TZ=Asia/Shanghai

COPY sources.list /etc/apt/sources.list

RUN apt-get update && \
    apt-get install -y --no-install-recommends curl gnupg2 ca-certificates && \
    curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg && \
    rm -rf /var/lib/apt/lists/*

RUN echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] https://mirrors.tuna.tsinghua.edu.cn/ros2/ubuntu jammy main" | tee /etc/apt/sources.list.d/ros2.list > /dev/null

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ros-humble-rosbag2-storage-mcap \
    ros-humble-rmw-cyclonedds-cpp \
    ros-humble-foxglove-bridge \
    libbluetooth-dev \
    libreadline-dev \
    libdbus-1-dev \
    libglib2.0-dev && \
    rm -rf /var/lib/apt/lists/*
