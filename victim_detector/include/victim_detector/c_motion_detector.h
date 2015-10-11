#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

#include <iostream>
#include <string>


#include "ros/ros.h"
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <opencv2/opencv.hpp>

// Dynamic Reconfigure Headers
#include <dynamic_reconfigure/server.h>
#include <victim_detector/MotionDetectorConfig.h>


namespace polymechanon_vision {


class MotionDetector
{
public:
	MotionDetector();
	~MotionDetector();


private:
	// ROS node's setup
	ros::NodeHandle _node;
	image_transport::Subscriber _subscriber_to_img_node;
	dynamic_reconfigure::Server<motion_detector::MotionDetectorConfig> _dyn_rec_server;

	bool _debugging;
	int _detection_mode;

	// Node's setup
	void imageCallback(const sensor_msgs::ImageConstPtr& msg);
	std::string loadTopic(const ros::NodeHandle& node, std::string topic_name = "/usb_camera/image_raw");	
	void loadDetectorSettings(const ros::NodeHandle& node);
	// void dynRecCallback(victim_detector::VictimDetectorConfig &config, uint32_t level);
	void dynRecCallback(motion_detector::MotionDetectorConfig &config, uint32_t level);














};

} // "namespace polymechanon_vision"
#endif // MOTION_DETECTOR_H















