#include "label_detector/scanners/c_hzl_scanner.h"

namespace polymechanon_vision {

HzlScanner::HzlScanner(shared_ptr<cv::Mat> input_image /* = nullptr */)
{	
	// if ( input_image != nullptr)    setImageToScan(input_image);
	if ( input_image != nullptr)    Scanner::setImageToScan(input_image);
	ROS_WARN("Hzl-Scanner created!");

	_canny_param1 = 100;
	_canny_param2 = 200;
	_label_side_length = 150;
	_match_method = 0;
	_contour_area_thres = 10000;

	_perspective_square = {
		Point2D(_label_side_length/2, 0),
		Point2D(_label_side_length, _label_side_length/2),
		Point2D(_label_side_length/2, _label_side_length),
		Point2D(0, _label_side_length/2)
	};

	loadLabels();

	// for ( auto tmpl: _labels_templates)
	// {

	// 	std::stringstream stream;
	// 	stream << "template -> " << tmpl.name;
	// 	std::string window = stream.str();
	// 	imshow(window, tmpl.gray_template_image);

	// }
}

HzlScanner::~HzlScanner() { 
	ROS_ERROR("Hzl-Scanner deleted!"); 
}

LabelType HzlScanner::getType() const
{
	return _type;
}

bool HzlScanner::scan()
{	
	if (_image_to_scan == nullptr ) throw std::runtime_error("[HzlScanner]-scan() : image's pointer is nullptr (use 'setImageToScan()'). ");


	static bool error_handler = true;
	if ( _labels_templates.size() == 0 )
	{	
		if ( error_handler ) {
			ROS_ERROR("[HzlScanner]-scan() : no labels' templates are loaded.");
			error_handler = false;
		}
		return false;
	} else if ( !error_handler ) 
		error_handler = true;



	_testing_image = (*_image_to_scan).clone();

	// calculateHistogram(_testing_image);


	cv::Mat camera_input = (*_image_to_scan).clone();

	// double contrast_ = 1.0;
	// int brightness_ = 1;

 //    for( int y = 0; y < (*_image_to_scan).rows; y++ )  { 
 //        for( int x = 0; x < (*_image_to_scan).cols; x++ )  { 
 //            for( int c = 0; c < 3; c++ )  {
 //                (*_image_to_scan).at<cv::Vec3b>(y,x)[c] = cv::saturate_cast<uchar>( contrast_*( (*_image_to_scan).at<cv::Vec3b>(y,x)[c] ) + brightness_ );
 //            }
 //        }
 //    }

	cv::Mat caminput_gray(_image_to_scan->size(), CV_MAKETYPE(_image_to_scan->depth(), 1));    // To hold Grayscale Image
	cv::Mat caminput_edges(caminput_gray.size(), CV_MAKETYPE(caminput_gray.depth(), 1));    // To hold Grayscale Image
	cv::Mat caminput_hsv;

	cvtColor(*_image_to_scan, caminput_gray, CV_RGB2GRAY);    // Convert Image captured from Image Input to GrayScale 
	Canny(caminput_gray, caminput_edges, _canny_param1 , _canny_param2, 3);    // Apply Canny edge detection on the img_gray image
	
	vector<vector<Point2D> > label_contours = findLabelContours(caminput_edges);

	// In case we didn't find any frame of a possible label.
	if ( label_contours.size() == 0 )    return false;

	vector<HzlLabel> labels = getHazardousLabels(_image_to_scan, label_contours);


	vector<vector<ContourPoint> > labels_contourpoints;
	for ( auto label: labels)
	{	
		labels_contourpoints.push_back(convertToContourPoint(label.contour));
	}

	// int counter = 0;
	// for ( auto label: labels)
	// {

	// 	std::stringstream stream_1;
	// 	std::stringstream stream_2;
	// 	stream_1 << "image" << counter;
	// 	stream_2 << "GRAY image" << counter;
	// 	std::string window1 = stream_1.str();
	// 	std::string window2 = stream_2.str();
	// 	imshow(window1, label.image);
	// 	imshow(window2, label.gray_image);

	// 	counter++;
	// }


	// for ( auto label: labels)    // ELEOS PALI
	for (size_t kk = 0; kk <labels.size(); kk++ )
	{	
		matchALabel(labels[kk]);
	}

	vector<HzlLabel> matched_labels;
	// for ( auto label: labels)
	for (size_t kk = 0; kk <labels.size(); kk++ )
	{	
		cout << " match id -> " << labels[kk].match << endl;
		if ( labels[kk].matched ){
			cout << _labels_templates[labels[kk].match].name << endl;
			matched_labels.push_back(labels[kk]);
		cout << "mphka" << endl;

			// calculateHistogram(labels[kk].image);
			calculateHistogram(_labels_templates[labels[kk].match].template_image);

		}
	}

	cout << "MATCHES - " << matched_labels.size() << endl;

	drawContours(_testing_image, labels_contourpoints);
	drawMatches(_testing_image, matched_labels );
	cv::imshow("wxwxw", _testing_image);
	// cv::imshow("MATCHES", camera_input);


	return false;
}

vector<vector<Point2D> > HzlScanner::getDetectedLabels()
{	
	vector<vector<Point2D> > detected_labels;
	return detected_labels;
}

bool HzlScanner::setParameters(int par1, int par2, int side_length, int match_mehod, int contour_area_thres)
{	
	_canny_param1 = par1;
	_canny_param2 = par2;
	_label_side_length = side_length;
	_match_method = match_mehod;
	_contour_area_thres = contour_area_thres;
	return true;
}

bool HzlScanner::loadLabels()
{   
	std::string labels_path(PACKAGE_PATH);
	// labels_path.append("config/hazardous_labels_images/labels_names.yaml");
	labels_path.append("/config/hazardous_labels_images/");
	std::string labels_names_file(labels_path);
	labels_names_file.append("labels_names.xml");


	vector< std::string > labels_names;

	cout << labels_names_file << endl;

	cv::FileStorage fs;

	try	{
		if ( fs.open(labels_names_file, cv::FileStorage::READ) ) {
			cv::FileNode names_node = fs["hazardous_labels"];
			cv::FileNodeIterator it = names_node.begin(), it_end = names_node.end();

			for( ; it != it_end; ++it ) 
				labels_names.push_back((*it));

		} else {
			ROS_ERROR("[HzlScanner]-loadLabels() : Missing xml file.");
		}
	}
	catch (const cv::Exception &e)	{
		ROS_ERROR("%s", e.what());
	}

	fs.release();

	_labels_templates.reserve(labels_names.size());

	for_each(begin(labels_names), end(labels_names),[&](const std::string& name){

		HzLabelTemplate new_label_template;
		// Set template name
		new_label_template.name = name;
		// Set template image
		std::string file_path(labels_path);
		file_path.append(name);
		file_path.append(".jpg");
		new_label_template.image = cv::imread(file_path);
		// Set template image
		new_label_template.template_image = new_label_template.image;
		cv::resize(new_label_template.template_image, new_label_template.template_image, cv::Size( _label_side_length, _label_side_length));
		cv::flip(new_label_template.template_image, new_label_template.template_image, 1);
		

		cv::cvtColor(new_label_template.template_image, new_label_template.gray_template_image, CV_BGR2GRAY);    // Convert Image captured from Image Input to GrayScale 
        // cv::equalizeHist(new_label_template.gray_template_image, new_label_template.gray_template_image);


		// Set template thumbnail
		new_label_template.thumbnail = new_label_template.image;
		cv::resize(new_label_template.thumbnail, new_label_template.thumbnail, cv::Size( 50, 50));


		_labels_templates.push_back(new_label_template);
	});

	ROS_WARN("[HzlScanner]-loadLabels() : labels loaded = %zu", _labels_templates.size() );
    return true;
}

/// Finds contours that meet the conditions of an alignment marker.
vector<vector<Point2D> > HzlScanner::findLabelContours(const cv::Mat& canny_image)
{
	vector<vector<ContourPoint> > contours;
	vector<vector<ContourPoint> > approximated_contours;
	vector<bool> contours_convexity;
	vector<cv::Vec4i> hierarchy;   
	cv::findContours( canny_image, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);    // Find contours with full hierarchy

	for_each(begin(contours), end(contours), [&](const vector<ContourPoint>& contour) {
		vector<ContourPoint> approximated_contour;
		
		cv::approxPolyDP(contour, approximated_contour, cv::arcLength(contour, true)*0.05, true);  
		approximated_contours.push_back(approximated_contour);
		contours_convexity.push_back( isContourConvex(approximated_contour) );
	});

	vector<vector<Point2D> > label_frames;    // To hold contours that meet the conditions of an alignment marker
	vector<Point2D> temp_vector;    

	if ( contours.size() == approximated_contours.size() ) {   // Just in case..

		for ( size_t k = 0; k < approximated_contours.size(); k++) {
			
			bool is_possible_label = approximated_contours[k].size() == 4 &&    // It is quadrilateral
									 contours_convexity[k] &&    // It is convex
									 hierarchy[k][2] != -1 &&    // It is enclosing another contour
									 approximated_contours[ hierarchy[k][2] ].size() == 4 &&    // The enclosing contour is quadrilateral
									 contours_convexity[hierarchy[k][2]] &&    // The enclosing contour is convex
									 contourArea(approximated_contours[hierarchy[k][2]]) > _contour_area_thres;    // Threshold of aeras

			if ( is_possible_label ) 
				label_frames.push_back( convertToPoint2D(approximated_contours[k]) );    // Contours that meet the conditions of a hazardous label frame are saved seperate
		}
	}
	return label_frames;
}

/// Finds contours that meet the conditions of an alignment marker.
vector<HzlLabel> HzlScanner::getHazardousLabels(shared_ptr<cv::Mat> image, const vector<vector<Point2D> >& contours)
{
	vector<HzlLabel> labels_to_return;

	for_each(begin(contours), end(contours), [&](const vector<Point2D>& contour){

		cv::Mat cropped;
		cv::Mat mask_to_crop = cv::Mat::zeros((*image).rows, (*image).cols, CV_8UC1);
		// cv::Mat label_raw;

		cropped = cv::Mat((*image).rows, (*image).cols, (*image).type());
		cropped.setTo(cv::Scalar(255,255,255));


		vector< vector< Point2D > > test = { contour };
		vector< vector< ContourPoint > > test_2 = { convertToContourPoint(contour) };

		cv::drawContours(mask_to_crop, test_2, -1, cv::Scalar(255), CV_FILLED);
		(*image).copyTo(cropped, mask_to_crop);

	    cv::Mat warp_matrix = getPerspectiveTransform(test[0], _perspective_square);

	    cv::Mat label_raw = cv::Mat::zeros(_label_side_length, _label_side_length, CV_8UC3 );
		label_raw.setTo(cv::Scalar(255,255,255));

	    warpPerspective(cropped, label_raw, warp_matrix, cv::Size(label_raw.cols, label_raw.rows));
	
		HzlLabel new_label;
		new_label.contour = contour;	    
		new_label.image = label_raw;	
		cv::Mat gray_label_raw;
		cv::cvtColor(label_raw, gray_label_raw, CV_BGR2GRAY);    // Convert Image captured from Image Input to GrayScale 
		new_label.gray_image = gray_label_raw;	    

		labels_to_return.push_back(new_label);
	});

	return labels_to_return;
}

bool HzlScanner::matchALabel(HzlLabel& label)
{	
	if ( _labels_templates.size() == 0 )    throw std::runtime_error( "[HzlScanner]-matchALabel() : no labels' templates are loaded" );	

    double temp_val = 0.0;
    double maximum_val = 0.0;

    int match;
    // match.quadrilateral_id = input_quadrilateral_id;


    for( int i = 0; i < _labels_templates.size(); i++ )  {
        temp_val = templateMatching(label.gray_image, _labels_templates[i].gray_template_image, _match_method);
        if ( temp_val > maximum_val)  {
            maximum_val = temp_val;
            match = i;

        }   
    }
    
    label.confidence = maximum_val;

    // cout << "maximum_val -> " << maximum_val << endl;
    // cout << "match -> " << match << endl;

    if ( maximum_val > 0.0) {
    	label.matched = true;
    	label.match = match;
    	cout << _labels_templates[match].name << endl;

    	    cout << "maximum_val -> " << maximum_val << endl;
    cout << "match -> " << match << endl;

		// imshow("image", label.gray_image);
		imshow("template image",  _labels_templates[match].gray_template_image);
		// imshow("template image",  _labels_templates[match].thumbnail);

    	return true;
    }

	label.matched = false;
    return false;
}

double HzlScanner::templateMatching(const cv::Mat &image_to_match, const cv::Mat &template_image, int whatmethod)
{   

    /// Localizing the best match with minMaxLoc
    double minVal; 
    double maxVal; 

    cv::Mat temp_image;

    copyMakeBorder( image_to_match, temp_image, 40, 40, 40, 40,cv::BORDER_CONSTANT, cv::Scalar(255,255,255) );

			imshow("template_image", template_image);
			imshow("temp_image", temp_image);


    cv::Mat result= temp_image.clone();
    /// Create the result matrix
    int result_cols =  temp_image.cols - template_image.cols + 1;
    int result_rows = temp_image.rows - template_image.rows + 1;

    result.create( result_rows, result_cols, CV_32FC1 );

    /// Do the Matching and Normalize
    matchTemplate( temp_image, template_image, result, whatmethod );
    normalize( result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat() );



    cv::Point minLoc; 
    cv::Point maxLoc;
    cv::Point matchLoc;

    cv::minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );

    // std::cout << maxVal << std::endl;

    /// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
    if ( whatmethod  == cv::TM_SQDIFF || whatmethod == cv::TM_SQDIFF_NORMED )    matchLoc = minLoc; 
    else  matchLoc = maxLoc; 
    // /// Show me what you got
    // cv::rectangle( temp_image, matchLoc, cv::Point( matchLoc.x + template_image.cols , matchLoc.y + template_image.rows ), cv::Scalar::all(0), 2, 8, 0 );
    // cv::rectangle( result, matchLoc, cv::Point( matchLoc.x + template_image.cols , matchLoc.y + template_image.rows ), cv::Scalar::all(0), 2, 8, 0 );

    double value_to_return;
    cv::Scalar mean, stddev;
    cv::meanStdDev(result, mean, stddev);

    value_to_return = mean[0];

    // if (DBG_MESSAGES_) std::cout << "DEVIATION " << stddev[0] << "|||\t||| MEAN " <<  mean[0] << std::endl;

    // return mean[0];
    // // return stddev[0];
    // return minVal;

				    // cout << " minVal = " << minVal << endl;
				    // cout << " maxVal = " << maxVal << endl;
				    cout << " mean[0] = " << mean[0] << endl;
				    cout << " stddev[0] = " << stddev[0] << endl;



    // if ( whatmethod  == cv::TM_SQDIFF || whatmethod == cv::TM_SQDIFF_NORMED )    value_to_return =  1 - value_to_return; 

    return value_to_return;
}

// bool HzlScanner::getHueHistgram()
// {	
// 	// hsv colors.
// 	// 0 red
// 	// 60 yellow
// 	// 120 green
// 	// 180 siel
// 	// 240 blue
// 	// 300 pink
// 	// 360 red

// 	            // cv::cvtColor(camera_input_,
//              //                hsv_camera_input,
//              //                CV_BGR2HSV);

// 	return true;
// }

// void HzlScanner::rotate_image_90n(const cv::Mat& src, const cv::Mat& dst, int angle)
// {   
//    if(src.data != dst.data){
//        src.copyTo(dst);
//    }

//    angle = ((angle / 90) % 4) * 90;

//    //0 : flip vertical; 1 flip horizontal
//    bool const flip_horizontal_or_vertical = angle > 0 ? 1 : 0;
//    int const number = std::abs(angle / 90);          

//    for(int i = 0; i != number; ++i){
//        cv::transpose(dst, dst);
//        cv::flip(dst, dst, flip_horizontal_or_vertical);
//    }
// }



// double HzlScanner::templateMatching(cv::Mat &input_image, const cv::Mat &template_image, int whatmethod)
// {   

//     // cv::Mat result;
//     // cvtColor(camera_input_,result,CV_RGB2GRAY);

//     // input_image = customMatching(input_image, template_image);

//     // cv::absdiff(input_image,template_image,input_image);


//     copyMakeBorder( input_image, input_image, 10, 10, 10, 10,cv::BORDER_CONSTANT, cv::Scalar(255,255,255) );

//     cv::Mat result= input_image.clone();
//     /// Create the result matrix
//     int result_cols =  input_image.cols - template_image.cols + 1;
//     int result_rows = input_image.rows - template_image.rows + 1;

//     result.create( result_rows, result_cols, CV_32FC1 );

//     /// Do the Matching and Normalize
//     matchTemplate( input_image, template_image, result, whatmethod );
//     normalize( result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat() );

//     /// Localizing the best match with minMaxLoc
//     double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
//     cv::Point matchLoc;

//     cv::minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );

//     // std::cout << maxVal << std::endl;

//     /// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
//     if ( whatmethod  == cv::TM_SQDIFF || whatmethod == cv::TM_SQDIFF_NORMED )    matchLoc = minLoc; 
//     else  matchLoc = maxLoc; 
//     /// Show me what you got
//     cv::rectangle( input_image, matchLoc, cv::Point( matchLoc.x + template_image.cols , matchLoc.y + template_image.rows ), cv::Scalar::all(0), 2, 8, 0 );
//     cv::rectangle( result, matchLoc, cv::Point( matchLoc.x + template_image.cols , matchLoc.y + template_image.rows ), cv::Scalar::all(0), 2, 8, 0 );


//     cv::Scalar mean, stddev;
//     cv::meanStdDev(result, mean, stddev);

//     // if (DBG_MESSAGES_) std::cout << "DEVIATION " << stddev[0] << "|||\t||| MEAN " <<  mean[0] << std::endl;

//     return mean[0];
//     // return stddev[0];
//     // return minVal;
// }

void HzlScanner::calculateHistogram(const cv::Mat& image)
{
	cv::Mat hsv_image;
	cv::cvtColor(image, hsv_image, CV_BGR2HSV);

	vector<cv::Mat> hsv_planes;
	cv::split(hsv_image, hsv_planes);

	// Establish the number of bins
	int histSize = 360;
	// Set the ranges ( for B,G,R) )
	float range[] = { 0, 360 } ;
	const float* histRange = { range };

	bool uniform = true; bool accumulate = false;

	cv::Mat hue_hist;
	calcHist( &hsv_planes[0], 1, 0, cv::Mat(), hue_hist, 1, &histSize, &histRange, uniform, accumulate );

	// Draw the histograms for B, G and R
	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound( (double) hist_w/histSize );

	cv::Mat histImage( hist_h, hist_w, CV_8UC3, cv::Scalar( 0,0,0) );
	cv::normalize(hue_hist, hue_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat() );


	for( int i = 1; i < histSize; i++ )
	{
		cv::line( histImage, cv::Point( bin_w*(i-1), hist_h - cvRound(hue_hist.at<float>(i-1)) ) ,
							 cv::Point( bin_w*(i), hist_h - cvRound(hue_hist.at<float>(i)) ),
							 // cv::Scalar( 255, 0, 0), 2, 8, 0  );
							 getBGRfromHUE(i), 2, 8, 0  );
							 // cv::Scalar( 255, 0, 0), 2, 8, 0  );
	}	

	cv::imshow("calcHist Demo", histImage );

}

cv::Scalar HzlScanner::getBGRfromHUE(const int& value)
{	
	cv::Scalar input_color;
	cv::Scalar output_color;


	if ( 0 <= value && value < 25) {    // RED
		output_color = { 0, 0, 255};
	} else if ( 25 <= value && value < 50) {    // ORANGE
		output_color = { 0, 162, 255};
	} else if ( 50 <= value && value < 70) {    // YELLOW
		output_color = { 0, 255, 255};	
	} else if ( 70 <= value && value < 160) {    // GREEN
		output_color = { 0, 255, 0};
	} else if ( 160 <= value && value < 200) {    // SIEL
		output_color = { 255, 255, 0};
	} else if ( 200 <= value && value < 280) {    // BLUE
		output_color = { 255, 0, 0};
	} else if ( 280 <= value && value < 330) {    // PINK
		output_color = { 255, 0, 255};
	} else if ( 330 <= value && value < 361) {    // SIEL
		output_color = { 0, 0, 255};
	}
	return output_color;

	// cv::Scalar color_to_return;
	// return color_to_return;
}

template<typename T>
vector<Point2D> HzlScanner::convertToPoint2D(const vector<T> input_vector)
{
	vector<Point2D> output_vector;

	for(size_t k = 0; k < input_vector.size(); k++)
	{
		output_vector.push_back(input_vector[k]);
	}

	return output_vector;
}

template<typename T>
vector<ContourPoint> HzlScanner::convertToContourPoint(const vector<T> input_vector)
{
	vector<ContourPoint> output_vector;

	for(size_t k = 0; k < input_vector.size(); k++)
	{
		output_vector.push_back(input_vector[k]);
	}

	return output_vector;
}


// Draw on input image the contours that this QrIdentifier is examining
void HzlScanner::drawContours(cv::Mat &inputimage, const vector<vector<ContourPoint> >& contours )
{	
	// cv::Scalar color = randomColor();
	cv::Scalar color = cv::Scalar(0,0,255);

	for( size_t i = 0; i < contours.size(); ++i )	{	// For each (of the 3) markers of our identifier
		cv::drawContours(inputimage, contours, i , color, 2, 8);		// Draw its contour (red)
		for( size_t j = 0; j < contours[i].size(); ++j )
		cv::circle(inputimage, contours[i][j], 2,  cv::Scalar(255,255,0), 2, 8, 0 );
	}
}


void HzlScanner::drawMatches(cv::Mat &inputimage, vector<HzlLabel> &labels)
{   

    for(auto label : labels )   {   // For each (of the 3) markers of our identifier
        cout << "?" << endl;
        cv::Rect roi( label.contour[0], cv::Size( _labels_templates[label.match].thumbnail.cols, _labels_templates[label.match].thumbnail.rows) );
        _labels_templates[label.match].thumbnail.copyTo( inputimage( roi ) );
    }



    // cv::circle( inputimage, mycenter, 2,  cv::Scalar(255,255,0), 2, 8, 0 );
}






///////////////////////// Debugging Functions /////////////////////////
bool HzlScanner::drawDetectedLabels(shared_ptr<cv::Mat> inputimage)
{	
	return false;
}

bool HzlScanner::drawDetectedLabels(cv::Mat &inputimage)
{
	return false;
}

cv::Mat HzlScanner::getImageOfDetectedLabels(const cv::Mat &inputimage)
{	
	cv::Mat image_to_draw = inputimage.clone();
	return image_to_draw;
}



} // "namespace polymechanon_vision"