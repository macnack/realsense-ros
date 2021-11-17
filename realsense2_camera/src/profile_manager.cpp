#include <profile_manager.h>
#include <regex>

using namespace realsense2_camera;
using namespace rs2;

ProfilesManager::ProfilesManager(std::shared_ptr<Parameters> parameters):
    _logger(rclcpp::get_logger("RealSenseCameraNode")),
    _params(parameters, _logger)
     {
     }

void ProfilesManager::clearParameters()
{
    while ( !_parameters_names.empty() )
    {
        auto name = _parameters_names.back();
        _params.getParameters()->removeParam(name);
        _parameters_names.pop_back();        
    }
}

std::string applyTemplateName(std::string template_name, stream_index_pair sip)
{
    const std::string stream_name(create_graph_resource_name(STREAM_NAME(sip)));
    char* param_name = new char[template_name.size() + stream_name.size()];
    sprintf(param_name, template_name.c_str(), stream_name.c_str());
    return std::string(param_name);
}

void ProfilesManager::registerSensorQOSParam(std::string template_name, 
                                          std::set<stream_index_pair> unique_sips, 
                                          std::map<stream_index_pair, std::shared_ptr<std::string> >& params, 
                                          std::string value)
{
    // For each pair of stream-index, Function add a QOS parameter to <params> and advertise it by <template_name>.
    // parameters in <params> are dynamically being updated. If invalid they are reverted.
    for (auto& sip : unique_sips)
    {
        std::string param_name = applyTemplateName(template_name, sip);
        params[sip] = std::make_shared<std::string>(value);
        std::shared_ptr<std::string> param = params[sip];
        rcl_interfaces::msg::ParameterDescriptor crnt_descriptor;
        crnt_descriptor.description = "Available options are:\n" + list_available_qos_strings();
        rclcpp::ParameterValue aa = _params.getParameters()->setParam(param_name, rclcpp::ParameterValue(value), [this, param](const rclcpp::Parameter& parameter)
                {
                    try
                    {
                        qos_string_to_qos(parameter.get_value<std::string>());
                        *param = parameter.get_value<std::string>();
                        ROS_WARN_STREAM("re-enable the stream for the change to take effect.");
                    }
                    catch(const std::exception& e)
                    {
                        ROS_ERROR_STREAM("Given value, " << parameter.get_value<std::string>() << " is unknown. Set ROS param back to: " << *param);
                        _params.getParameters()->queueSetRosValue(parameter.get_name(), *param);
                    }
                }, crnt_descriptor);
        _parameters_names.push_back(param_name);
    }
}

template<class T>
void ProfilesManager::registerSensorUpdateParam(std::string template_name, 
                                                std::set<stream_index_pair> unique_sips, 
                                                std::map<stream_index_pair, std::shared_ptr<T> >& params, 
                                                T value, 
                                                std::function<void()> update_sensor_func)
{
    // This function registers parameters that their modification requires a sensor update.
    // For each pair of stream-index, Function add a parameter to <params> and advertise it by <template_name>.
    // parameters in <params> are dynamically being updated.
    for (auto& sip : unique_sips)
    {
        std::string param_name = applyTemplateName(template_name, sip);
        params[sip] = std::make_shared<T>(value);
        std::shared_ptr<T> param = params[sip];
        rclcpp::ParameterValue aa = _params.getParameters()->setParam(param_name, rclcpp::ParameterValue(value), [param, update_sensor_func](const rclcpp::Parameter& parameter)
                {
                    *param = parameter.get_value<T>();
                    update_sensor_func();
                });
        _parameters_names.push_back(param_name);
    }
}

template void ProfilesManager::registerSensorUpdateParam<bool>(std::string template_name, std::set<stream_index_pair> unique_sips, std::map<stream_index_pair, std::shared_ptr<bool> >& params, bool value, std::function<void()> update_sensor_func);
template void ProfilesManager::registerSensorUpdateParam<double>(std::string template_name, std::set<stream_index_pair> unique_sips, std::map<stream_index_pair, std::shared_ptr<double> >& params, double value, std::function<void()> update_sensor_func);


bool ProfilesManager::isTypeExist()
{
    return (!_enabled_profiles.empty());
}

rs2::stream_profile ProfilesManager::getDefaultProfile()
{
    rs2::stream_profile default_profile;
    if (_all_profiles.empty()) 
        throw std::runtime_error("Wrong commands sequence. No profiles set.");

    for (auto profile : _all_profiles)
    {
        stream_index_pair sip(profile.stream_type(), profile.stream_index());
        if (profile.is_default())
        {
            default_profile = profile;
            break;
        }
    }
    if (!(default_profile.get()))
        throw std::runtime_error("No default profile found");

    return default_profile;
}

void ProfilesManager::addWantedProfiles(std::vector<rs2::stream_profile>& wanted_profiles)
{    
    std::map<stream_index_pair, bool> found_sips;
    std::map<stream_index_pair, rs2::stream_profile> default_profiles;
    for (auto profile : _all_profiles)
    {
        stream_index_pair sip(profile.stream_type(), profile.stream_index());
        if (!(*_enabled_profiles[sip])) continue;
        if (found_sips.find(sip) == found_sips.end())
        {
            found_sips[sip] = false;
        }
        else
        {
            if (found_sips.at(sip) == true) continue;
        }
        if (profile.is_default())
        {
            default_profiles[sip] = profile;
        }
        if (isWantedProfile(profile))
        {
            wanted_profiles.push_back(profile);
            found_sips[sip] = true;
            ROS_DEBUG_STREAM("Found profile for " << ros_stream_to_string(sip.first) << ":" << sip.second);
        }
    }
}

std::string ProfilesManager::profile_string(const rs2::stream_profile& profile)
{
    std::stringstream profile_str;
    if (profile.is<rs2::video_stream_profile>())
    {
        auto video_profile = profile.as<rs2::video_stream_profile>();
        profile_str << "stream_type: " << ros_stream_to_string(video_profile.stream_type()) << "(" << video_profile.stream_index() << ")" <<
                       ", Format: " << video_profile.format() <<
                       ", Width: " << video_profile.width() <<
                       ", Height: " << video_profile.height() <<
                       ", FPS: " << video_profile.fps();
    }
    else
    {
        profile_str << "stream_type: " << ros_stream_to_string(profile.stream_type()) << "(" << profile.stream_index() << ")" <<
                       "Format: " << profile.format() <<
                       ", FPS: " << profile.fps();
    }
    return profile_str.str();
}

bool ProfilesManager::hasSIP(const stream_index_pair& sip) const
{
    return (_enabled_profiles.find(sip) != _enabled_profiles.end());
}

rmw_qos_profile_t ProfilesManager::getQOS(const stream_index_pair& sip) const
{
    return qos_string_to_qos(*(_profiles_image_qos_str.at(sip)));
}

rmw_qos_profile_t ProfilesManager::getInfoQOS(const stream_index_pair& sip) const
{
    return qos_string_to_qos(*(_profiles_info_qos_str.at(sip)));
}

VideoProfilesManager::VideoProfilesManager(std::shared_ptr<Parameters> parameters,
                                           const std::string& module_name):
    ProfilesManager(parameters),
    _module_name(module_name)
{
    _allowed_formats[RS2_STREAM_DEPTH] = RS2_FORMAT_Z16;
    _allowed_formats[RS2_STREAM_INFRARED] = RS2_FORMAT_Y8;
}

std::string VideoProfilesManager::wanted_profile_string(stream_index_pair sip)
{
    std::stringstream str;
    str << STREAM_NAME(sip) << " with width: " << _width << ", " << "height: " << _height << ", fps: " << _fps;
    return str.str();
}

bool VideoProfilesManager::isWantedProfile(const rs2::stream_profile& profile, const int width, const int height, const int fps)
{
    if (!profile.is<rs2::video_stream_profile>())
        return false;
    auto video_profile = profile.as<rs2::video_stream_profile>();
    ROS_DEBUG_STREAM("Sensor profile: " << profile_string(profile));

    return ((video_profile.width() == width) &&
            (video_profile.height() == height) &&
            (video_profile.fps() == fps) &&
            (_allowed_formats.find(video_profile.stream_type()) == _allowed_formats.end() || video_profile.format() == _allowed_formats[video_profile.stream_type()] ));
}

bool VideoProfilesManager::isWantedProfile(const rs2::stream_profile& profile)
{
    return isWantedProfile(profile, _width, _height, _fps);
}

void VideoProfilesManager::registerProfileParameters(std::vector<stream_profile> all_profiles, std::function<void()> update_sensor_func)
{
    std::set<stream_index_pair> checked_sips;
    for (auto& profile : all_profiles)
    {
        if (!profile.is<video_stream_profile>()) continue;
        ROS_DEBUG_STREAM("Register profile: " << profile_string(profile));
        _all_profiles.push_back(profile);
        stream_index_pair sip(profile.stream_type(), profile.stream_index());
        checked_sips.insert(sip);
    }
    if (!checked_sips.empty())
    {
        ROS_DEBUG_STREAM(__LINE__ << ": _enabled_profiles.size(): " << _enabled_profiles.size());
        registerSensorUpdateParam("enable_%s", checked_sips, _enabled_profiles, true, update_sensor_func);
        registerSensorQOSParam("%s_qos", checked_sips, _profiles_image_qos_str, IMAGE_QOS);
        registerSensorQOSParam("%s_info_qos", checked_sips, _profiles_info_qos_str, DEFAULT_QOS);
        for (auto& sip : checked_sips)
        {
            ROS_DEBUG_STREAM(__LINE__ << ": _enabled_profiles[" << ros_stream_to_string(sip.first) << ":" << sip.second << "]: " << *(_enabled_profiles[sip]));
        }

        registerVideoSensorParams();
    }
}

void VideoProfilesManager::registerVideoSensorParams()
{
    // Set default values:
    rs2::stream_profile default_profile = getDefaultProfile();
    auto video_profile = default_profile.as<rs2::video_stream_profile>();

    _width = video_profile.width();
    _height = video_profile.height();
    _fps = video_profile.fps();

    // Register ROS parameter:
    std::string param_name(_module_name + ".profile");
    rcl_interfaces::msg::ParameterDescriptor crnt_descriptor;
    crnt_descriptor.description = "Available options are: DORON\n";
    std::stringstream crnt_profile_str;
    crnt_profile_str << _width << "x" << _height << "x" << _fps;
    rclcpp::ParameterValue aa = _params.getParameters()->setParam(param_name, rclcpp::ParameterValue(crnt_profile_str.str()), [this](const rclcpp::Parameter& parameter)
            {
                std::regex self_regex("\\s*([0-9]+)\\s*[xX,]\\s*([0-9]+)\\s*[xX,]\\s*([0-9]+)\\s*", std::regex_constants::ECMAScript);
                std::smatch match;
                std::string profile_str(parameter.get_value<std::string>());
                bool found = std::regex_match(profile_str, match, self_regex);
                bool request_default(false);
                if (found)
                {
                    int temp_width(std::stoi(match[1])), temp_height(std::stoi(match[2])), temp_fps(std::stoi(match[3]));
                    if (temp_width <= 0 || temp_height <= 0 || temp_fps <= 0)
                    {
                        found = false;
                        request_default = true;
                    }
                    else
                    {
                        for (const auto& profile : _all_profiles)
                        {
                            found = false;
                            if (isWantedProfile(profile, temp_width, temp_height, temp_fps))
                            {
                                _width = temp_width;
                                _height = temp_height;
                                _fps = temp_fps;
                                found = true;
                                ROS_WARN_STREAM("re-enable the stream for the change to take effect.");
                                break;
                            }
                        }
                    }
                }
                if (!found)
                {
                    std::stringstream crnt_profile_str;
                    crnt_profile_str << _width << "x" << _height << "x" << _fps;
                    if (request_default)
                    {
                        ROS_INFO_STREAM("Set ROS param " << parameter.get_name() << " to default: " << crnt_profile_str.str());
                    }
                    else
                    {
                        ROS_ERROR_STREAM("Given value, " << parameter.get_value<std::string>() << " is invalid. Set ROS param back to: " << crnt_profile_str.str());
                    }
                    _params.getParameters()->queueSetRosValue(parameter.get_name(), crnt_profile_str.str());
                }
            }, crnt_descriptor);
    _parameters_names.push_back(param_name);
}

///////////////////////////////////////////////////////////////////////////////////////

bool MotionProfilesManager::isWantedProfile(const rs2::stream_profile& profile)
{
    stream_index_pair stream(profile.stream_type(), profile.stream_index());
    return (profile.fps() == *(_fps[stream]));
}

void MotionProfilesManager::registerProfileParameters(std::vector<stream_profile> all_profiles, std::function<void()> update_sensor_func)
{
    std::set<stream_index_pair> checked_sips;
    for (auto& profile : all_profiles)
    {
        if (!profile.is<motion_stream_profile>()) continue;
        ROS_DEBUG_STREAM("Register profile: " << profile_string(profile));
        _all_profiles.push_back(profile);
        stream_index_pair sip(profile.stream_type(), profile.stream_index());
        checked_sips.insert(sip);
    }
    registerSensorUpdateParam("enable_%s", checked_sips, _enabled_profiles, true, update_sensor_func);
    registerSensorUpdateParam("%s_fps", checked_sips, _fps, 0.0, update_sensor_func);
    registerSensorQOSParam("%s_qos", checked_sips, _profiles_image_qos_str, HID_QOS);
    registerSensorQOSParam("%s_info_qos", checked_sips, _profiles_info_qos_str, DEFAULT_QOS);
}

std::string MotionProfilesManager::wanted_profile_string(stream_index_pair sip)
{
    std::stringstream str;
    str << STREAM_NAME(sip) << " with fps: " << *(_fps[sip]);
    return str.str();
}

///////////////////////////////////////////////////////////////////////////////////////

void PoseProfilesManager::registerProfileParameters(std::vector<stream_profile> all_profiles, std::function<void()> update_sensor_func)
{
    std::set<stream_index_pair> checked_sips;
    for (auto& profile : all_profiles)
    {
        if (!profile.is<pose_stream_profile>()) continue;
        _all_profiles.push_back(profile);
        stream_index_pair sip(profile.stream_type(), profile.stream_index());
        checked_sips.insert(sip);
    }
    registerSensorUpdateParam("enable_%s", checked_sips, _enabled_profiles, true, update_sensor_func);
    registerSensorUpdateParam("%s_fps", checked_sips, _fps, 0.0, update_sensor_func);
    registerSensorQOSParam("%s_qos", checked_sips, _profiles_image_qos_str, HID_QOS);
    registerSensorQOSParam("%s_info_qos", checked_sips, _profiles_info_qos_str, DEFAULT_QOS);
}
