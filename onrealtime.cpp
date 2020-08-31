// ------------------------- OpenPose C++ API Tutorial - Example 16 - Custom Output -------------------------
// Synchronous mode: ideal for production integration. It provides the fastest results with respect to runtime
// performance.
// In this function, the user can implement its own way to render/display/storage the results.

// Third-party dependencies
#include <opencv2/opencv.hpp>
// Command-line user interface
#define OPENPOSE_FLAGS_DISABLE_DISPLAY
#include <openpose/flags.hpp>
// OpenPose dependencies
#include <openpose/headers.hpp>
#include <cpr/cpr.h>

// Custom OpenPose flags
// Display
DEFINE_bool(no_display, false,
    "Enable to disable the visual display.");

// This worker will just read and return all the jpg files in a directory
class WUserOutput : public op::WorkerConsumer<std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>>
{
public:
    void initializationOnThread() {}

    void workConsumer(const std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr)
    {
        try
        {
            // User's displaying/saving/other processing here
             //datumPtr->cvOutputData: rendered frame with pose or heatmaps
                // datumPtr->poseKeypoints: Array<float> with the estimated pose
            if (datumsPtr != nullptr && !datumsPtr->empty())
            {
                // Show in command line the resulting pose keypoints for body, face and hands
                op::opLog("\nKeypoints:");
                // Accesing each element of the keypoints
                const auto& poseKeypoints = datumsPtr->at(0)->poseKeypoints;
                op::opLog("Person pose keypoints:");
                for (auto person = 0; person < poseKeypoints.getSize(0); person++)
                {
                    op::opLog("Person " + std::to_string(person) );
                    auto y2 = poseKeypoints[{person, 2, 1}];
                    auto y3 = poseKeypoints[{person, 3, 1}];
                    auto y4 = poseKeypoints[{person, 4, 1}];
                    auto y5 = poseKeypoints[{person, 5, 1}];
                    auto y6 = poseKeypoints[{person, 6, 1}];
                    auto y7 = poseKeypoints[{person, 7, 1}];

                    op::opLog("y2 = " + std::to_string(y2) + " y3 = " + std::to_string(y3) + " y4 = " + std::to_string(y4), op::Priority::High);
                    op::opLog("y5 = " + std::to_string(y5) + " y6 = " + std::to_string(y6) + " y7 = " + std::to_string(y7), op::Priority::High);
                    if (y2 - y3 >= 0 && y3 - y4 > 0 || y5 - y6 >= 0 && y6 - y7 > 0)
                    {
                        //opLog("right hands up" , op::Priority::High);
                        if (y2 - y3 >= 0 && y3 - y4 > 0 && y5 - y6 >= 0 && y6 - y7 > 0)
                        {
                            opLog("both hands up", op::Priority::High);
                            auto r = cpr::Put(cpr::Url{ "https://pose-detection-8bab0.firebaseio.com/pose-detection.json" },
                                cpr::Body{ R"({"Id":1, "Action":"Both hands up"})" },
                                cpr::Header{ { "Content-Type", "json" } });
                            break;
                        }
                        if (y5 - y6 >= 0 && y6 - y7 > 0)
                        {
                            opLog("left hands up", op::Priority::High);
                            auto r = cpr::Put(cpr::Url{ "https://pose-detection-8bab0.firebaseio.com/pose-detection.json" },
                                cpr::Body{ R"({"Id":2, "Action":"left hand up"})" },
                                cpr::Header{ { "Content-Type", "json" } });

                        }
                        if (y2 - y3 >= 0 && y3 - y4 > 0)
                        {
                            opLog("right hands up", op::Priority::High);
                            auto r = cpr::Put(cpr::Url{ "https://pose-detection-8bab0.firebaseio.com/pose-detection.json" },
                                cpr::Body{ R"({"Id":3, "Name":"right hand up"})" },
                                cpr::Header{ { "Content-Type", "json" } });

                        }

                    }
                    else {
                        opLog("both hands down", op::Priority::High);
                        auto r = cpr::Put(cpr::Url{ "https://pose-detection-8bab0.firebaseio.com/pose-detection.json" },
                            cpr::Body{ R"({"Id":4, "Action":"both hands down"})" },
                            cpr::Header{ { "Content-Type", "json" } });
                    }

                }
                op::opLog(" ");
                // Heatmaps
                const auto& poseHeatMaps = datumsPtr->at(0)->poseHeatMaps;
                if (!poseHeatMaps.empty())
                {
                    op::opLog("Pose heatmaps size: [" + std::to_string(poseHeatMaps.getSize(0)) + ", "
                        + std::to_string(poseHeatMaps.getSize(1)) + ", "
                        + std::to_string(poseHeatMaps.getSize(2)) + "]");
                }

                // Display results (if enabled)
                if (!FLAGS_no_display)
                {
                    // Display rendered output image
                    const cv::Mat cvMat = OP_OP2CVCONSTMAT(datumsPtr->at(0)->cvOutputData);
                    cv::imshow(OPEN_POSE_NAME_AND_VERSION + " - Tutorial C++ API", cvMat);
                    // Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
                    const char key = (char)cv::waitKey(1);
                    if (key == 27)
                        this->stop();
                }
            }
        }
        catch (const std::exception & e)
        {
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
};

void configureWrapper(op::Wrapper& opWrapper)
{
    try
    {
        // Configuring OpenPose

        // logging_level
        op::checkBool(
            0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.",
            __LINE__, __FUNCTION__, __FILE__);
        op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
        op::Profiler::setDefaultX(FLAGS_profile_speed);

        // Applying user defined configuration - GFlags to program variables
        // producerType
        op::ProducerType producerType;
        op::String producerString;
        std::tie(producerType, producerString) = op::flagsToProducer(
            op::String(FLAGS_image_dir), op::String(FLAGS_video), op::String(FLAGS_ip_camera), FLAGS_camera,
            FLAGS_flir_camera, FLAGS_flir_camera_index);
        // cameraSize
        const auto cameraSize = op::flagsToPoint(op::String(FLAGS_camera_resolution), "-1x-1");
        // outputSize
        const auto outputSize = op::flagsToPoint(op::String(FLAGS_output_resolution), "-1x-1");
        // netInputSize
        const auto netInputSize = op::flagsToPoint(op::String(FLAGS_net_resolution), "-1x368");
        // faceNetInputSize
        const auto faceNetInputSize = op::flagsToPoint(op::String(FLAGS_face_net_resolution), "368x368 (multiples of 16)");
        // handNetInputSize
        const auto handNetInputSize = op::flagsToPoint(op::String(FLAGS_hand_net_resolution), "368x368 (multiples of 16)");
        // poseMode
        const auto poseMode = op::flagsToPoseMode(FLAGS_body);
        // poseModel
        const auto poseModel = op::flagsToPoseModel(op::String(FLAGS_model_pose));
        // JSON saving
        if (!FLAGS_write_keypoint.empty())
            op::opLog(
                "Flag `write_keypoint` is deprecated and will eventually be removed. Please, use `write_json`"
                " instead.", op::Priority::Max);
        // keypointScaleMode
        const auto keypointScaleMode = op::flagsToScaleMode(FLAGS_keypoint_scale);
        // heatmaps to add
        const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg,
            FLAGS_heatmaps_add_PAFs);
        const auto heatMapScaleMode = op::flagsToHeatMapScaleMode(FLAGS_heatmaps_scale);
        // >1 camera view?
        const auto multipleView = (FLAGS_3d || FLAGS_3d_views > 1 || FLAGS_flir_camera);
        // Enabling Google Logging
        const bool enableGoogleLogging = true;

        // Initializing the user custom classes
        // GUI (Display)
        auto wUserOutput = std::make_shared<WUserOutput>();
        // Add custom processing
        const auto workerOutputOnNewThread = true;
        opWrapper.setWorker(op::WorkerType::Output, wUserOutput, workerOutputOnNewThread);

        // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
        const op::WrapperStructPose wrapperStructPose{
            poseMode, netInputSize, outputSize, keypointScaleMode, FLAGS_num_gpu, FLAGS_num_gpu_start,
            FLAGS_scale_number, (float)FLAGS_scale_gap, op::flagsToRenderMode(FLAGS_render_pose, multipleView),
            poseModel, !FLAGS_disable_blending, (float)FLAGS_alpha_pose, (float)FLAGS_alpha_heatmap,
            FLAGS_part_to_show, op::String(FLAGS_model_folder), heatMapTypes, heatMapScaleMode, FLAGS_part_candidates,
            (float)FLAGS_render_threshold, FLAGS_number_people_max, FLAGS_maximize_positives, FLAGS_fps_max,
            op::String(FLAGS_prototxt_path), op::String(FLAGS_caffemodel_path),
            (float)FLAGS_upsampling_ratio, enableGoogleLogging };
        opWrapper.configure(wrapperStructPose);
        // Extra functionality configuration (use op::WrapperStructExtra{} to disable it)
        const op::WrapperStructExtra wrapperStructExtra{
            FLAGS_3d, FLAGS_3d_min_views, FLAGS_identification, FLAGS_tracking, FLAGS_ik_threads };
        opWrapper.configure(wrapperStructExtra);
        // Producer (use default to disable any input)
        const op::WrapperStructInput wrapperStructInput{
            producerType, producerString, FLAGS_frame_first, FLAGS_frame_step, FLAGS_frame_last,
            FLAGS_process_real_time, FLAGS_frame_flip, FLAGS_frame_rotate, FLAGS_frames_repeat,
            cameraSize, op::String(FLAGS_camera_parameter_path), FLAGS_frame_undistort, FLAGS_3d_views };
        opWrapper.configure(wrapperStructInput);
        // Output (comment or use default argument to disable any output)
        const op::WrapperStructOutput wrapperStructOutput{
            FLAGS_cli_verbose, op::String(FLAGS_write_keypoint), op::stringToDataFormat(FLAGS_write_keypoint_format),
            op::String(FLAGS_write_json), op::String(FLAGS_write_coco_json), FLAGS_write_coco_json_variants,
            FLAGS_write_coco_json_variant, op::String(FLAGS_write_images), op::String(FLAGS_write_images_format),
            op::String(FLAGS_write_video), FLAGS_write_video_fps, FLAGS_write_video_with_audio,
            op::String(FLAGS_write_heatmaps), op::String(FLAGS_write_heatmaps_format), op::String(FLAGS_write_video_3d),
            op::String(FLAGS_write_video_adam), op::String(FLAGS_write_bvh), op::String(FLAGS_udp_host),
            op::String(FLAGS_udp_port) };
        opWrapper.configure(wrapperStructOutput);
        // No GUI. Equivalent to: opWrapper.configure(op::WrapperStructGui{});
        // Set to single-thread (for sequential processing and/or debugging and/or reducing latency)
        if (FLAGS_disable_multi_thread)
            opWrapper.disableMultiThreading();
    }
    catch (const std::exception & e)
    {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    }
}

int tutorialApiCpp()
{
    try
    {
        op::opLog("Starting OpenPose demo...", op::Priority::High);
        const auto opTimer = op::getTimerInit();

        // OpenPose wrapper
        op::opLog("Configuring OpenPose...", op::Priority::High);
        op::Wrapper opWrapper;
        configureWrapper(opWrapper);

        // Start, run, and stop processing - exec() blocks this thread until OpenPose wrapper has finished
        op::opLog("Starting thread(s)...", op::Priority::High);
        opWrapper.exec();

        // Measuring total time
        op::printTime(opTimer, "OpenPose demo successfully finished. Total time: ", " seconds.", op::Priority::High);

        // Return
        return 0;
    }
    catch (const std::exception&)
    {
        return -1;
    }
}

int main(int argc, char* argv[])
{
    // Parsing command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Running tutorialApiCpp
    return tutorialApiCpp();
}
