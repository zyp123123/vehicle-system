# 🚗 Qt 嵌入式车载终端系统 (IVI System)

## 📖 项目介绍
本项目是一个基于 C++11 和 Qt 框架开发的高性能嵌入式车载信息娱乐系统（IVI）。项目打通了从 Linux 内核驱动层（GPIO、中断、字符设备）、中间件层（V4L2、OpenCV、FFmpeg、MQTT）、到 GUI 应用层（混合 Web 编程、异步多线程 UI）的完整技术链路。

项目完美适配 ARM 架构开发板（如 i.MX6ULL 等），同时兼容 x86 PC 端调试。包含了地图导航、哨兵模式、倒车影像、远程物联网控制、多媒体播放等 10 余个核心车载 App。

---

## 🚀 核心功能模块 (Apps)

### 🗺️ 智能导航 (Map)
基于 QWebEngineView 与高德地图 Web JS API 深度整合，实现 Native 与 Web 的数据双向绑定。

### 🛡️ 哨兵模式 (Sentinel)
底层驱动 SR501 人体红外传感器，结合高频定时器防抖机制，实现异常入侵自动触发 V4L2 摄像头抓拍并存档。

### 📷 倒车影像 (Parking)
结合 HC-SR04 超声波测距驱动与摄像头，实现实时测距与碰撞警告（LED 硬件联动）。

### 📡 远程互联 (Remote)
集成 MQTT 协议，实现车机状态上报（DHT11 温湿度）及云端下发控制指令（车灯/蜂鸣器）。

### 🖼️ 极速相册 (Album)
基于 QThreadPool 与 QtConcurrent 实现图片流式异步缩略图加载，支持手势滑动与全屏预览，彻底告别 UI 卡顿。

### 🎵 多媒体娱乐 (Media)
包含纯自绘 UI 的音视频播放器（Music / Video），支持本地媒体异步扫描、进度拖拽、列表循环。

### ⏰ 车载闹钟 (Alarm)
使用 SQLite 数据库持久化存储数据，并纯手绘实现了车载滚动时间选择器 (NumberPicker) 与平滑开关 (SwitchButton)。

---

## 🛠 核心技术栈

- GUI 框架：Qt 5/6 (Widgets, QStackedWidget, QPropertyAnimation, QtCharts)
- Web 混合开发：QWebEngineView, QWebChannel (C++ 与 JavaScript 双向通信)
- 多媒体与视觉：V4L2 (底层视频采集), OpenCV (imdecode/cvtColor 图像处理), FFmpeg (QProcess 调起 RTMP 推流), QMediaPlayer
- 并发与异步：QtConcurrent::run, QFutureWatcher, 自定义 AppThreadPool, QThread
- 网络与云平台：TCP/IP (QTcpServer/QTcpSocket), HTTP API (QNetworkAccessManager), MQTT (QMqttClient)
- Linux 驱动与 OS：字符设备驱动开发、Platform 平台总线、Device Tree (设备树)、内核中断 (request_irq)、并发锁 (mutex)

---

## 💡 核心难点与技术突破 (面试亮点)

### 1. 突破 UI 假死瓶颈：高并发异步相册加载
挑战：
在 ARM 开发板上，如果直接在主线程加载大量高清图片并生成缩略图，会导致车机 UI 瞬间卡死甚至触发系统 Watchdog 重启。

解决方案：
引入 QtConcurrent 并封装了全局线程池 AppThreadPool（严格限制最大活动线程数防止 CPU 满载）。通过 QImageReader 在后台线程提取图像，利用 QFutureWatcher 监听任务完成状态，并通过 QMetaObject::invokeMethod 安全跨线程回调给主 UI 进行图标渲染。

成果：
即使相册目录下有数百张高清抓拍图片，UI 依然保持 60FPS 的丝滑滑动体验。

---

### 2. Native 与 Web 的无缝融合：地图导航的双向通信
挑战：
Qt 自带的地图模块扩展性较差，而使用浏览器控件套壳加载高德地图时，C++ 无法直接获取或控制网页内部的导航状态和自动补全数据。

解决方案：
基于 QWebChannel 实现了 C++ 对象到 JS 运行时的注入。当用户在 Qt QLineEdit 输入地址时，C++ 通过信号实时触发 JS 的高德 API 搜索；JS 获取到地理坐标 JSON 后，回调 C++ 函数，由 Qt 渲染底层 QListView 补全列表。

成果：
结合了 Web 地图的强大生态与 Qt Native 控件的高效输入交互，并通过自定义 TCP 服务端将渲染好的地图帧实时下发给移动端。

---

### 3. 底层性能压榨：零延迟 V4L2 采集与硬件推流
挑战：
使用 QCamera 封装层在 ARM 板上往往存在极高延迟，且无法满足哨兵模式的瞬时抓拍与远程监控推流需求。

解决方案：
直接重写底层的 V4L2 (Video for Linux 2) 采集类。通过 mmap 内存映射零拷贝获取 MJPEG 帧，并利用 OpenCV 进行内存态的 imdecode 与色彩转换。对于直播推流，不占用主程序计算资源，而是利用 QProcess 拉起 FFmpeg 进程向 RTMP 服务器推流。

成果：
彻底消除了预览画面的拖影现象，内存占用降低了 40%，实现本地监控与云端直播并发运行。

---

### 4. 解决内核态的计算崩溃：Linux 驱动的 64位除法安全
挑战：
在编写 HC-SR04 超声波驱动的中断处理函数时，纳秒转毫米的公式需要用到 64 位整型的除法。在 32 位 ARM Linux 内核中直接使用 / 运算符会导致编译报 __aeabi_uldivmod 缺失错误，甚至引发内核 Panic。

解决方案：
查阅 Linux 内核源码规范，引入 <linux/math64.h>，使用内核专属的安全除法 API div_u64() 替换了标准运算符。同时，为 DHT11 时序驱动加入了 mutex 互斥锁，防止高频多进程读取导致的时序错乱。

成果：
成功编写了高稳定性的内核驱动（.ko 模块），并在 Qt Settings App 中通过 Shell 指令实现了驱动的动态加载与热插拔。

---

## 📂 核心项目结构

```plaintext
├── apps/                  # 车载核心应用层 (App)
│   ├── mappage.cpp        # 混合地图应用 (QWebChannel 核心)
│   ├── monitor.cpp        # 监控应用 (V4L2 + FFmpeg推流)
│   ├── sentinel.cpp       # 哨兵模式 (SR501 红外抓拍)
│   ├── remote.cpp         # MQTT 远程互联与控制
│   ├── album.cpp          # 异步多线程相册
│   └── alarm.cpp          # 闹钟与本地 SQLite 存储
├── tools/                 # 自定义底层工具与控件库
│   ├── v4l2capture.cpp    # 封装的 Linux 视频采集类
│   ├── appthreadpool.cpp  # 全局安全线程池
│   ├── slidepage.cpp      # 手势滑动分页容器
│   ├── numberpicker.cpp   # 纯手绘的滚轮数字选择器
│   └── switchbutton.cpp   # 纯手绘的动画开关控件
├── driver/                # 硬件驱动层 (C语言 Linux Kernel Module)
│   ├── dht11.c            # 温湿度传感器 Platform 驱动 (带互斥锁)
│   ├── hcsr04.c           # 超声波传感器驱动 (中断响应 + pinctrl)
│   └── sr501.c            # 人体红外字符设备驱动
└── resources/             # 界面 QSS 样式表与图片资源
```

---

## ⚙️ 编译与运行指南

本项目支持 x86 (Ubuntu 本地模拟) 与 ARM (交叉编译部署) 双平台自动适配。

### 1. 依赖安装

```bash
sudo apt-get install qt5-default qtwebengine5-dev libqt5charts5-dev libqt5mqtt5-dev
sudo apt-get install libopencv-dev ffmpeg
```

### 2. 编译主程序

```bash
qmake vehicle-system.pro
make -j4
```

### 3. 编译驱动

进入 driver/ 各目录下执行：

```bash
# 请确保 Makefile 中指向了正确的 KERNELDIR
make
# 产出 dht11.ko, hcsr04.ko, sr501.ko
```

### 4. 运行

```bash
./vehicle-system
```
