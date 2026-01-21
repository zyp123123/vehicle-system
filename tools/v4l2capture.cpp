#include "v4l2capture.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cstring>
#include <iostream>
#include <linux/videodev2.h>

V4L2Capture::V4L2Capture()
{
    fd = -1;
    buffers = nullptr;
    n_buffers = 0;
}

V4L2Capture::~V4L2Capture()
{
    closeDevice();
}

bool V4L2Capture::openDevice(const std::string &devName, int width, int height)
{
    frameWidth  = width;
    frameHeight = height;

    fd = ::open(devName.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        std::cerr << "Cannot open device: " << devName << std::endl;
        return false;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width  = frameWidth;
    fmt.fmt.pix.height = frameHeight;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;   // ★ 使用 MJPEG
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        std::cerr << "VIDIOC_S_FMT failed" << std::endl;
        return false;
    }

    return initMMap() && startCapturing();
}


void V4L2Capture::closeDevice()
{
    stopCapturing();
    uninitMMap();

    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

bool V4L2Capture::initMMap()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        return false;
    }

    buffers = (buffer*)calloc(req.count, sizeof(*buffers));
    n_buffers = req.count;

    for (unsigned int i = 0; i < req.count; ++i)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        ioctl(fd, VIDIOC_QUERYBUF, &buf);

        buffers[i].length = buf.length;
        buffers[i].start  = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd, buf.m.offset);

        ioctl(fd, VIDIOC_QBUF, &buf);
    }

    return true;
}

bool V4L2Capture::startCapturing()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return (ioctl(fd, VIDIOC_STREAMON, &type) != -1);
}

void V4L2Capture::stopCapturing()
{
    if (fd != -1) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
    }
}

void V4L2Capture::uninitMMap()
{
    if(buffers) {
        for (unsigned i = 0; i < n_buffers; i++) {
            munmap(buffers[i].start, buffers[i].length);
        }
        free(buffers);
        buffers = nullptr;
        n_buffers = 0;
    }
}

bool V4L2Capture::readFrame(cv::Mat &frame)
{
    if(fd < 0) return false;

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
        return false;

    // ★ 这里不是 YUYV，是 JPEG，需要解码
    std::vector<uchar> encoded((uchar*)buffers[buf.index].start,
                               (uchar*)buffers[buf.index].start + buffers[buf.index].length);
    frame = cv::imdecode(encoded, cv::IMREAD_COLOR);   // MJPEG → BGR

    ioctl(fd, VIDIOC_QBUF, &buf);
    return !frame.empty();
}

