ARG BASE_IMAGE=ros:humble-perception

FROM ${BASE_IMAGE} as librealsense-builder

# install ros package
RUN apt-get update \
    && apt-get install -qq -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libusb-1.0-0-dev \
    pkg-config \
    libgtk-3-dev \
    libglfw3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \    
    curl \
    python3 \
    python3-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src

RUN curl -L -o librealsense-2.50.0.tar.gz https://github.com/IntelRealSense/librealsense/archive/refs/tags/v2.50.0.tar.gz \
    && tar -xzvf librealsense-2.50.0.tar.gz \
    && rm librealsense-2.50.0.tar.gz \
    && mv librealsense-2.50.0 /usr/src/librealsense

RUN cd /usr/src/librealsense \
    && mkdir build \
    && cd build \
    && cmake \
    -DCMAKE_C_FLAGS_RELEASE="${CMAKE_C_FLAGS_RELEASE} -s" \
    -DCMAKE_CXX_FLAGS_RELEASE="${CMAKE_CXX_FLAGS_RELEASE} -s" \
    -DCMAKE_INSTALL_PREFIX=/opt/librealsense \    
    -DBUILD_GRAPHICAL_EXAMPLES=OFF \
    -DBUILD_PYTHON_BINDINGS:bool=true \
    -DCMAKE_BUILD_TYPE=Release ../ \
    && make -j$(nproc) all \
    && make install

FROM ${BASE_IMAGE} as librealsense

COPY --from=librealsense-builder /opt/librealsense /usr/local/
COPY --from=librealsense-builder /usr/lib/python3/dist-packages/pyrealsense2 /usr/lib/python3/dist-packages/pyrealsense2
COPY --from=librealsense-builder /usr/src/librealsense/config/99-realsense-libusb.rules /etc/udev/rules.d/
COPY ./update_realsense_ros_CMakeLists.txt /tmp/update_realsense_ros_CMakeLists.txt
ENV PYTHONPATH=${PYTHONPATH}:/usr/local/lib

RUN apt-get update \
    && apt-get install -y --no-install-recommends \	
    libusb-1.0-0 \
    udev \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

ENV WS_DIR="/ros2_ws"
WORKDIR ${WS_DIR}

SHELL ["/bin/bash", "-c"]
RUN apt-get update -y \
    && apt-get install -y \
    ros-${ROS_DISTRO}-rviz2 \
    && mkdir src \
    && cd src \
    && git clone https://github.com/macnack/realsense-ros.git -b ros2 \
    && cd ..

RUN apt-get install -y python3-rosdep \
    && source /opt/ros/${ROS_DISTRO}/setup.bash \
    && rm /etc/ros/rosdep/sources.list.d/20-default.list \
    && rosdep init \
    && rosdep update \
    && rosdep install -i --from-path src --rosdistro ${ROS_DISTRO} --skip-keys=librealsense2 -y \
    && colcon build
    
CMD ["bash"]
