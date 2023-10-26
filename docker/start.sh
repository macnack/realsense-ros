docker run -it --rm \
        -v /dev:/dev \
        --device-cgroup-rule "c 81:* rmw" \
        --device-cgroup-rule "c 189:* rmw" \
        --gpus all \
        --env="DISPLAY=$DISPLAY" \
        --env="QT_X11_NO_MITSHM=1" \
        --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
        --env="XAUTHORITY=$XAUTH" \
        --volume="$XAUTH:$XAUTH" \
        --volume="/home/mackop/work/intention/ROS/Shared:/root/Shared:rw" \
        --env="NVIDIA_VISIBLE_DEVICES=all" \
        --env="NVIDIA_DRIVER_CAPABILITIES=all" \
        --privileged \
        --network=host \
        realsense_ros2