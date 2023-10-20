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

"""Launch realsense2_camera node without rviz2."""
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.substitutions import ThisLaunchFileDir
from launch.launch_description_sources import PythonLaunchDescriptionSource
import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.absolute()))
import rs_launch

local_parameters = [{'name': 'camera_name1', 'default': 'D435i', 'description': 'camera unique name'},
                    {'name': 'device_type1', 'default': 'l515', 'description': 'choose device by type'},
                    {'name': 'enable_pointcloud1', 'default': 'true', 'description': 'enable pointcloud'},
                    {'name': 'motion_module.global_time_enabled', 'default' : 'true', 'description' : 'enable global time'},
                    {'name': 'l500_depth_sensor.global_time_enabled', 'default' : 'true', 'description' : 'enable global time'},
                    {'name': 'rgb_camera.global_time_enabled', 'default': 'true', 'description': 'enable global time'},
                    {'name': 'enable_fisheye11', 'default': 'true', 'description': 'enable fisheye1 stream'},
                    {'name': 'enable_fisheye12', 'default': 'true', 'description': 'enable fisheye2 stream'},
                    {'name': 'depth_width1', 'default': '640', 'description': 'des'},
                    {'name': 'depth_height1', 'default': '480', 'description': 'des'},
                    {'name': 'depth_fps1', 'default': '15', 'description': 'des'},
                    {'name': 'color_width1', 'default': '640', 'description': 'des'},
                    {'name': 'color_height1', 'default': '480', 'description': 'des'},
                    {'name': 'color_fps1', 'default': '15', 'description': 'des'},
                    {'name': 'enable_gyro1', 'default': 'true', 'description': 'enable gyro'},
                    {'name': 'enable_accel1', 'default': 'true', 'description': 'enable acc'},
                    {'name': 'camera_name2', 'default': 'T265', 'description': 'camera unique name'},
                    {'name': 'device_type2', 'default': 't265', 'description': 'choose device by type'},
                    {'name': 'enable_gyro2', 'default': 'true', 'description': 'enable gyro'},
                    {'name': 'enable_accel2', 'default': 'true', 'description': 'enable acc'},
                    {'name': 'enable_pose2', 'default': 'true', 'description': 'enable pose stream'},
                   ]


def generate_launch_description():
    return LaunchDescription(
        rs_launch.declare_configurable_parameters(local_parameters) + 
        [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([ThisLaunchFileDir(), '/rs_octomap_launch.py']),
            launch_arguments=rs_launch.set_configurable_parameters(local_parameters).items(),
        ),
    ])
