#ifndef V4L2CAPTURE_H
#define V4L2CAPTURE_H

#include <string>
#include <opencv2/opencv.hpp>

struct buffer {
    void* start;
    size_t length;
};

class V4L2Capture
{
public:
    V4L2Capture();
    ~V4L2Capture();

    bool openDevice(const std::string &devName, int width, int height);
    void closeDevice();
    bool readFrame(cv::Mat &frame);

private:
    int fd;
    buffer* buffers;
    unsigned int n_buffers;
    int frameWidth;
    int frameHeight;

    bool initMMap();
    bool startCapturing();
    void stopCapturing();
    void uninitMMap();
};

#endif // V4L2CAPTURE_H
