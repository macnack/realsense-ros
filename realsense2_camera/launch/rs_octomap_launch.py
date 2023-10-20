# Copyright (c) 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# DESCRIPTION #
# ----------- #
# Use this launch file to launch 2 devices.
# The Parameters available for definition in the command line for each camera are described in rs_launch.configurable_parameters
# For each device, the parameter name was changed to include an index.
# For example: to set camera_name for device1 set parameter camera_name1.
# command line example:
# ros2 launch realsense2_camera rs_multi_camera_launch.py camera_name1:=my_D435 device_type1:=d435 camera_name2:=my_d415 device_type2:=d415

"""Launch realsense2_camera node."""
import rs_launch
import copy
from os.path import join
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, ThisLaunchFileDir
from launch.launch_description_sources import PythonLaunchDescriptionSource
import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.absolute()))

local_parameters = [{'name': 'camera_name10', 'default': 'camera1', 'description': 'camera unique name'},
                    {'name': 'camera_name2', 'default': 'camera2',
                        'description': 'camera unique name'},
                    ]


def set_configurable_parameters(local_params):
    return dict([(param['original_name'], LaunchConfiguration(param['name'])) for param in local_params])


def duplicate_params(general_params, posix):
    local_params = copy.deepcopy(general_params)
    for param in local_params:
        param['original_name'] = param['name']
        param['name'] += posix
    return local_params


def generate_launch_description():
    params1 = duplicate_params(rs_launch.configurable_parameters, '1')
    params2 = duplicate_params(rs_launch.configurable_parameters, '2')
    tf2_t265_d435i = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        arguments=["-0.009", "0.021", "0.027", "0.005",
                   "-0.018", "0.000", "T265_link", "D435i_link"]
    )
    tf2_t265_pose_frame = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        arguments=["0.0", "0.0", "0.0", "0.0", "0.0",
                   "0.0", "T265_pose_frame", "T265_link"]
    )
    rviz_config_dir = join(get_package_share_directory(
        'realsense2_camera'), 'rviz', 'octomap.rviz')
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', rviz_config_dir],
        parameters=[{'use_sim_time': False}]
    )
    octomap_node = Node(
        package='octomap_server2',
        executable='octomap_server',
        output='screen',
        remappings=[('cloud_in', '/D435i/depth/color/points')],
        parameters=[{'resolution': LaunchConfiguration('resolution'),
                     'frame_id': LaunchConfiguration('frame_id')}]
    )
    return LaunchDescription(
        rs_launch.declare_configurable_parameters(local_parameters) +
        rs_launch.declare_configurable_parameters(params1) +
        rs_launch.declare_configurable_parameters(params2) +
        [
            DeclareLaunchArgument('resolution', default_value='0.10'),
            DeclareLaunchArgument('frame_id', default_value='T265_odom_frame'),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    [ThisLaunchFileDir(), '/rs_launch.py']),
                launch_arguments=set_configurable_parameters(params1).items(),
            ),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    [ThisLaunchFileDir(), '/rs_launch.py']),
                launch_arguments=set_configurable_parameters(params2).items(),
            ),
            tf2_t265_d435i,
            tf2_t265_pose_frame,
            octomap_node,
            rviz_node
        ])
