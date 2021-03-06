#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
//#include <Windows.h>
#include "headers.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <omp.h>

using namespace cv;
using namespace std;

void CropFacialComponent(cv::Mat image, cv::Mat_<double>& shape){
    double min_x,min_y,max_x,max_y;
    // holly shit
    min_x=2000;min_y=2000;max_x=0;max_y=0;
    cv::Mat eyesAndBrow,nose,mouth;
    for (int i = 17; i < 48; i++){//left_eye_part
        if (i<36 && i>=27) continue;
        // cv::circle(image, cv::Point2f(shape(i, 0), shape(i, 1)), 2, (255));
        if (shape(i,0) < min_x) min_x = shape(i,0);
        if (shape(i,1) < min_y) min_y = shape(i,1);
        if (shape(i,0) > max_x) max_x = shape(i,0);
        if (shape(i,1) > max_y) max_y = shape(i,1);
    }
    cv::Point2d start = Point2d(min_x,min_y);
    cv::Rect eyesAndBrowPart(min_x,min_y,max_x-min_x,max_y-min_y);
    image(eyesAndBrowPart).copyTo(eyesAndBrow);
    min_x=2000;min_y=2000;max_x=0;max_y=0;
    for (int i = 27; i < 36; i++){//left_eye_part
        // cv::circle(image, cv::Point2f(shape(i, 0), shape(i, 1)), 2, (255));
        if (shape(i,0) < min_x) min_x = shape(i,0);
        if (shape(i,1) < min_y) min_y = shape(i,1);
        if (shape(i,0) > max_x) max_x = shape(i,0);
        if (shape(i,1) > max_y) max_y = shape(i,1);
    }
    start = Point2d(min_x,min_y);
    cv::Rect nosePart(min_x,min_y,max_x-min_x,max_y-min_y);
    image(nosePart).copyTo(nose);
    min_x=2000;min_y=2000;max_x=0;max_y=0;
    for (int i = 49; i < 68; i++){//left_eye_part
        // cv::circle(image, cv::Point2f(shape(i, 0), shape(i, 1)), 2, (255));
        if (shape(i,0) < min_x) min_x = shape(i,0);
        if (shape(i,1) < min_y) min_y = shape(i,1);
        if (shape(i,0) > max_x) max_x = shape(i,0);
        if (shape(i,1) > max_y) max_y = shape(i,1);
    }
    start = Point2d(min_x,min_y);
    cv::Rect mouthPart(min_x,min_y,max_x-min_x,max_y-min_y);
    image(mouthPart).copyTo(mouth);
    // cv::imshow("bbox",bbox);
    // cv::namedWindow("left_eye",1);
    // cv::imshow("left_eye", croppedFacialComponent);
    cv::imwrite("./eyesAndBrow.png",eyesAndBrow); 
    cv::imwrite("./nose.png",nose); 
    cv::imwrite("./mouth.png",mouth); 
    std::cout << "Facial parts have been save in current folder" << std::endl;
    // cv::waitKey(0);
}

void DrawPredictedImage(cv::Mat_<uchar> image, cv::Mat_<double>& shape, BoundingBox& bbox){
    for (int i = 0; i < shape.rows; i++){
        cv::circle(image, cv::Point2f(shape(i, 0), shape(i, 1)), 2, (255));
    }
    cv::Point2f start=Point (bbox.start_x,bbox.start_y);
    cv::Point2f center = Point (bbox.start_x + bbox.width,bbox.start_y+bbox.height);
    cv::rectangle(image, start, center, cv::Scalar(0,0,255));
    // cv::imshow("bbox",bbox);
    cv::imshow("show image", image);
    cv::waitKey(0);
}

void Test(const char* config_file_path){
	cout << "parsing config_file: " << config_file_path << endl;

    ifstream fin;
    fin.open(config_file_path, ifstream::in);
	std::string model_name;
    fin >> model_name;
    cout << "model name is: " << model_name << endl;
	bool images_has_ground_truth = false;
	fin >> images_has_ground_truth;
	if (images_has_ground_truth) {
		cout << "the image lists must have ground_truth_shapes!\n" << endl;
	}
	else{
		cout << "the image lists does not have ground_truth_shapes!!!\n" << endl;
	}

	int path_num;
    fin >> path_num;
    cout << "reading testing images paths: " << endl;
	std::vector<std::string> image_path_prefixes;
    std::vector<std::string> image_lists;
    for (int i = 0; i < path_num; i++) {
        string s;
        fin >> s;
        cout << s << endl;
        image_path_prefixes.push_back(s);
        fin >> s;
        cout << s << endl;
        image_lists.push_back(s);
    }

	cout << "parsing config file done\n" << endl;
	CascadeRegressor cas_load;
	cas_load.LoadCascadeRegressor(model_name);
	cout << "load model done\n" << endl;
	std::vector<cv::Mat_<uchar> > images;
    std::vector<cv::Mat> colorImages;
	std::vector<cv::Mat_<double> > ground_truth_shapes;
    std::vector<BoundingBox> bboxes;

    string image_path,pts_path,image_file_name,image_pts_name,path_prefix=image_path_prefixes[0];
    int size = path_prefix.size();
    for(int i=size-2;i!=0;i--){
        if(path_prefix[i] == '/'){
            path_prefix = path_prefix.substr(0,i+1);
            path_prefix += "result";
            break;
        }
    }

    std::cout << "\nLoading test dataset..." << std::endl;
    ofstream _fout;
    ifstream finput;
    vector<string> result_files;
    if (images_has_ground_truth) {
        LoadImages(images, ground_truth_shapes, bboxes, image_path_prefixes, image_lists);
        double error = 0.0;
        for (int i = 0; i < images.size(); i++){
            cv::Mat_<double> current_shape = ReProjection(cas_load.params_.mean_shape_, bboxes[i]);
            // This is predict shape which is exactly what we need.
            cv::Mat_<double> res = cas_load.Predict(images[i], current_shape, bboxes[i]);//, ground_truth_shapes[i]);
            // DrawPredictedImage(images[i], res, bboxes[i]);
            // std::cout << res << std::endl;
            double e = CalculateError(ground_truth_shapes[i], res);

            finput.open((image_lists[i]).c_str(), std::ifstream::in);
            while (finput >> image_file_name >> image_pts_name){
                if (path_prefix[path_prefix.size()-1] == '/') {
                    image_path = path_prefix + image_file_name;
                    pts_path = path_prefix + image_pts_name;
                }
                else{
                    image_path = path_prefix + "/" + image_file_name;
                    pts_path = path_prefix + "/" + image_pts_name;
                }
                image_path += ".result";
                result_files.push_back(image_path);
            }

            _fout.open(result_files[i].c_str(),ofstream::out);
            if (_fout){
                _fout << res;
                _fout.close();
            }

            // std::cout << "error:" << e << std::endl;
            error += e;
        }
        std::cout << "The results are stored in: "<< path_prefix << endl;
        std::cout << "error: " << error << ", mean error: " << error/images.size() << std::endl;
    }
    else{
        LoadImages(colorImages,images, bboxes, image_path_prefixes, image_lists);
        for (int i = 0; i < images.size(); i++){
            cv::Mat_<double> current_shape = ReProjection(cas_load.params_.mean_shape_, bboxes[i]);
            cv::Mat_<double> res = cas_load.Predict(images[i], current_shape, bboxes[i]);//, ground_truth_shapes[i]);
            finput.open(image_lists[i].c_str(),ifstream::in);
            while (finput >> image_file_name){
                if (path_prefix[path_prefix.size()-1] == '/') {
                    image_path = path_prefix + image_file_name;
                }
                else{
                    image_path = path_prefix + "/" + image_file_name;
                }
                image_path += ".result";
                result_files.push_back(image_path);
            }
            _fout.open(result_files[i].c_str(),ofstream::out);
            // std::cout << result.c_str() << std::endl;
            if (_fout){
                _fout << res;
                _fout.close();
            }
            // DrawPredictedImage(images[i], res, bboxes[i]);
            CropFacialComponent(colorImages[i],res);
        }
        std::cout << "The results are stored in: "<< path_prefix << endl;
    }
}

void Train(const char* config_file_path){

    cout << "parsing config_file: " << config_file_path << endl;

    ifstream fin;
    fin.open(config_file_path, ifstream::in);
    std::string model_name;
    fin >> model_name;
    cout << "\nmodel name is: " << model_name << endl;
    Parameters params = Parameters();
    fin >> params.local_features_num_
        >> params.landmarks_num_per_face_
        >> params.regressor_stages_
        >> params.tree_depth_
        >> params.trees_num_per_forest_
        >> params.initial_guess_
        >> params.overlap_;

    std::vector<double> local_radius_by_stage;
    local_radius_by_stage.resize(params.regressor_stages_);
    for (int i = 0; i < params.regressor_stages_; i++){
        fin >> local_radius_by_stage[i];
    }
    params.local_radius_by_stage_ = local_radius_by_stage;
    params.output();

    int path_num;
    fin >> path_num;
    cout << "\nreading training images paths: " << endl;

    std::vector<std::string> image_path_prefixes;
    std::vector<std::string> image_lists;
    for (int i = 0; i < path_num; i++) {
        string s;
        fin >> s;
        cout << s << endl;
        image_path_prefixes.push_back(s);
        fin >> s;
        cout << s << endl;
        image_lists.push_back(s);
    }

    fin >> path_num;
    cout << "\nreading validation images paths: " << endl;
    std::vector<std::string> val_image_path_prefixes;
    std::vector<std::string> val_image_lists;
    for (int i = 0; i < path_num; i++) {
        string s;
        fin >> s;
        cout << s << endl;
        val_image_path_prefixes.push_back(s);
        fin >> s;
        cout << s << endl;
        val_image_lists.push_back(s);
    }

    cout << "parsing config file done\n" << endl;


    std::vector<cv::Mat_<uchar> > images;
    std::vector<cv::Mat_<double> > ground_truth_shapes;
    std::vector<BoundingBox> bboxes;

    std::vector<cv::Mat_<uchar> > val_images;
    std::vector<cv::Mat_<double> > val_ground_truth_shapes;
    std::vector<BoundingBox> val_bboxes;
    std::cout << "Loading training dataset..." << std::endl;
    LoadImages(images, ground_truth_shapes, bboxes, image_path_prefixes, image_lists);
    if (val_image_lists.size() > 0) {
        std::cout << "\nLoading validation dataset..." << std::endl;
        LoadImages(val_images, val_ground_truth_shapes, val_bboxes, val_image_path_prefixes, val_image_lists);
    }
    // else{
    // 	std::cout << "your validation dataset is 0" << std::endl;
    // }

    params.mean_shape_ = GetMeanShape(ground_truth_shapes, bboxes);
    CascadeRegressor cas_reg;
    cas_reg.val_bboxes_ = val_bboxes;
    cas_reg.val_images_ = val_images;
    cas_reg.val_ground_truth_shapes_ = val_ground_truth_shapes;

    cas_reg.Train(images, ground_truth_shapes, bboxes, params);
    std::cout << "finish training, start to saving the model..." << std::endl;
    std::cout << "model name: " << model_name << std::endl;
    cas_reg.SaveCascadeRegressor(model_name);
    std::cout << "save the model successfully\n" << std::endl;
}

int main(int argc, char* argv[])
{
    std::cout << "\nuse [./application train train_config_file] to train models" << std::endl;
    std::cout << "    [./application test test_config_file] to test images\n\n" << std::endl;

    if (argc == 3) {
        if (strcmp(argv[1], "train") == 0)
        {
            Train(argv[2]);
            return 0;
        }
        if (strcmp(argv[1], "test") == 0)
        {
            Test(argv[2]);
            return 0;
        }
    }
    else {
        std::cout << "\nWRONG!!!" << std::endl;
    }

    return 0;
}
