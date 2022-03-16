#include <iostream>
#include <vector>
#include <exception>
#include <chrono>
#include <thread>

#include <chronoptics/tof/kea_camera.hpp>
#include <chronoptics/tof/user_config.hpp>
#include <chronoptics/tof/camera_config.hpp>
#include <chronoptics/tof/gige_interface.hpp>
#include <chronoptics/tof/usb_interface.hpp>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

#include "cxxopts.hpp"
#include "colormap.hpp"

namespace tof = chronoptics::tof;
using namespace std::chrono_literals;

void get_frame(std::vector<tof::Data> &frames, const tof::FrameType frame_type, tof::Data &out_frame)
{
    for (tof::Data &frame : frames)
    {
        if (frame.frame_type() == frame_type)
        {
            out_frame = std::move(frame);
            break;
        }
    }
    return;
}

int main(int argc, char **argv)
{
    cxxopts::Options options("Kea on-camera display",
                             "Display the output of Kea camera to the HDMI port");

    cv::String intensity_name = "Intensity";
    cv::String rad_name = "Radial";
    cv::String bgr_name = "BGR";
    cv::String bgr_proj_name = "BGR Projected";
    bool bgr = false;
    bool bgr_projected = false;
    float dmax;
    float fps;
    int binning = 1;
    bool on_camera = false;
    std::string serial;

    options.add_options()("h, help", "Help")("l, list", "List all cameras discovered")("bgr", "Display the colour image", cxxopts::value<bool>(bgr)->default_value("false"))("bgr_projected", "Display the projected BGR Image", cxxopts::value<bool>(bgr_projected)->default_value("false"))("dmax", "Maximum distance", cxxopts::value<float>(dmax)->default_value("30.0"))("fps", "Depth frames per second", cxxopts::value<float>(fps)->default_value("15.0"))("serial", "Camera Serial Number", cxxopts::value<std::string>(serial));

    auto result = options.parse(argc, argv);

    if (result.count("h") || result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (result.count("l") || result.count("list"))
    {
        std::vector<tof::DiscoveryMessage> cameras;
        std::vector<tof::UsbDevice> devices;

        devices = tof::usb_device_discover();
        tof::GigeInterface gige;
        cameras = gige.discover();

        for (std::size_t i = 0; i < devices.size(); i++)
        {
            std::cout << devices[i].serial() << " - usb" << std::endl;
        }

        for (std::size_t i = 0; i < cameras.size(); i++)
        {
            auto &msg = cameras[i];
            std::cout << msg.serial() << " " << msg.ip() << std::endl;
        }
        return 0;
    }

    std::vector<tof::FrameType> types = {tof::FrameType::RADIAL, tof::FrameType::INTENSITY};
    if (bgr)
    {
        types.push_back(tof::FrameType::BGR);
    }
    if (bgr_projected)
    {
        types.push_back(tof::FrameType::BGR_PROJECTED);
    }

    // The way to color the depth image with a JET colour map
    std::array<std::array<uint8_t, 3>, 256> jet;
    populate_colormap(jet);

    try
    {

        auto proc = tof::ProcessingConfig();
        proc.set_calibration_enabled(true);
        proc.set_intensity_scale(5.0f);
        if (on_camera == true)
        {
            proc.set_temporal_sigma(1.0);
        }

        tof::KeaCamera cam(tof::ProcessingConfig{}, serial);

        // Configure the camera
        tof::UserConfig user{};
        user.set_fps(fps);
        user.set_integration_time(tof::IntegrationTime::SHORT);
        user.set_max_distance(dmax);
        user.set_environment(tof::ImagingEnvironment::SUNLIGHT);
        user.set_strategy(tof::Strategy::BALANCED);

        auto config = user.to_camera_config(cam);
        for (size_t n = 0; n < config.frame_size(); n++)
        {
            // Flip the output to be displayed correctly with 1/4" Mounting hole
            config.set_binning(n, binning);
        }

        // Generate the ToF ISP filtering settings
        proc = config.default_processing();
        proc.set_intensity_scale(5.0);

        // Set the camera settings
        cam.set_camera_config(config);
        cam.set_process_config(proc);
        if (on_camera)
        {
            if (cam.on_camera_processing_capable())
            {
                cam.set_on_camera_processing(on_camera);
            }
        }

        // Check bgr is avaliable on this camera.
        bool bgr_avaliable = false;
        bool bgr_proj_avaliable = false;
        auto stream_list = cam.get_stream_list();
        for (auto &stream : stream_list)
        {
            if (stream.frame_type() == tof::FrameType::BGR)
            {
                bgr_avaliable = true;
            }
            else if (stream.frame_type() == tof::FrameType::BGR_PROJECTED)
            {
                bgr_proj_avaliable = true;
            }
        }
        if (bgr)
        {
            bgr = bgr_avaliable;
        }
        if (bgr_projected)
        {
            bgr_projected = bgr_proj_avaliable;
        }

        size_t nstreams = tof::select_streams(cam, types);
        if (nstreams == 0)
        {
            throw std::runtime_error("Did not select any streams from the camera!");
        }

        if (bgr)
        {
            cv::namedWindow(bgr_name, cv::WINDOW_AUTOSIZE);
        }
        if (bgr_projected)
        {
            cv::namedWindow(bgr_proj_name, cv::WINDOW_AUTOSIZE);
        }
        cv::namedWindow(intensity_name, cv::WINDOW_AUTOSIZE);
        cv::namedWindow(rad_name, cv::WINDOW_AUTOSIZE);

        cam.start();
        cv::Mat disp_img;
        cv::Mat rad_disp;
        cv::Mat rgb_disp;

        while (cam.is_streaming())
        {
            std::vector<tof::Data> frames = cam.get_frames();

            // std::this_thread::sleep_for(200ms);
            for (auto &frame : frames)
            {
                if (frame.frame_type() == tof::FrameType::RADIAL)
                {
                    cv::Mat rad_img(frame.rows(), frame.cols(), CV_16UC1, frame.data());
                    convert_image(rad_img, rad_disp, 0, 65535, jet);
                    cv::flip(rad_disp, rad_disp, 0);

                    cv::imshow(rad_name, rad_disp);
                }
                else if (frame.frame_type() == tof::FrameType::BGR)
                {
                    if (bgr)
                    {
                        cv::Mat bgr_img(frame.rows(), frame.cols(), CV_8UC3, frame.data());
                        bgr_img.copyTo(rgb_disp);
                        cv::flip(rgb_disp, rgb_disp, 0);
                        cv::imshow(bgr_name, rgb_disp);
                    }
                }
                else if (frame.frame_type() == tof::FrameType::INTENSITY)
                {
                    cv::Mat intensity_img(frame.rows(), frame.cols(), CV_8UC1, frame.data());
                    cv::flip(intensity_img, intensity_img, 0);
                    cv::imshow(intensity_name, intensity_img);
                }
                else if (frame.frame_type() == tof::FrameType::BGR_PROJECTED)
                {
                    cv::Mat bgr_proj_img(frame.rows(), frame.cols(), CV_8UC3, frame.data());
                    cv::flip(bgr_proj_img, bgr_proj_img, 0);
                    cv::imshow(bgr_proj_name, bgr_proj_img);
                }
            }

            // If escape is selected then close
            if (cv::waitKey(10) == 27)
            {
                cam.stop();
            }
        }
    }
    catch (std::exception &e)
    {
        cv::destroyAllWindows();
        std::cout << e.what() << std::endl;
        return -1;
    }
    cv::destroyAllWindows();
    return 0;
}