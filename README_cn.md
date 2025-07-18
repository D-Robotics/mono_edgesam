[English](./README.md) | 简体中文

Getting Started with mono edgesam
=======

# 功能介绍

mono_edgesam package是基于 Edge SAM 量化部署的使用示例。图像数据来源于本地图片回灌和订阅到的image msg。SAM 依赖检测框输入进行分割, 并分割检测框中的目标, 无需指定目标的类别信息, 仅需提供框。

本示例中, 我们提供了两种部署展示方式:
- 固定框分割：固定了检测框（图片中央）用以分割。
- 订阅框分割：订阅上游检测网络输出的检测框信息, 对框中的信息进行分割。

# 开发环境

- 编程语言: C/C++
- 开发平台: X5
- 系统版本：Ubuntu 22.04
- 编译工具链: Linux GCC 11.4.0

# 编译

- X5版本：支持在X5 Ubuntu系统上编译和在PC上使用docker交叉编译两种方式。

同时支持通过编译选项控制编译pkg的依赖和pkg的功能。

## 依赖库

- opencv:3.4.5

ros package：

- dnn node
- cv_bridge
- sensor_msgs
- hbm_img_msgs
- ai_msgs

hbm_img_msgs为自定义的图片消息格式, 用于shared mem场景下的图片传输, hbm_img_msgs pkg定义在hobot_msgs中, 因此如果使用shared mem进行图片传输, 需要依赖此pkg。


## 编译选项

1、SHARED_MEM

- shared mem（共享内存传输）使能开关, 默认打开（ON）, 编译时使用-DSHARED_MEM=OFF命令关闭。
- 如果打开, 编译和运行会依赖hbm_img_msgs pkg, 并且需要使用tros进行编译。
- 如果关闭, 编译和运行不依赖hbm_img_msgs pkg, 支持使用原生ros和tros进行编译。
- 对于shared mem通信方式, 当前只支持订阅nv12格式图片。

## X5 Ubuntu系统上编译

1、编译环境确认

- 板端已安装X5 Ubuntu系统。
- 当前编译终端已设置TogetherROS环境变量：`source PATH/setup.bash`。其中PATH为TogetherROS的安装路径。
- 已安装ROS2编译工具colcon。安装的ROS不包含编译工具colcon, 需要手动安装colcon。colcon安装命令：`pip install -U colcon-common-extensions`
- 已编译dnn node package

2、编译

- 编译命令：`colcon build --packages-select mono_edgesam`

## docker交叉编译 X5版本

1、编译环境确认

- 在docker中编译, 并且docker中已经安装好TogetherROS。docker安装、交叉编译说明、TogetherROS编译和部署说明详见机器人开发平台robot_dev_config repo中的README.md。
- 已编译dnn node package
- 已编译hbm_img_msgs package（编译方法见Dependency部分）

2、编译

- 编译命令：

  ```shell
  # RDK X5
  bash robot_dev_config/build.sh -p X5 -s mono_edgesam
  ```

- 编译选项中默认打开了shared mem通信方式。

## 注意事项


# 使用介绍

## 依赖

- mipi_cam package：发布图片msg
- usb_cam package：发布图片msg
- websocket package：渲染图片和ai感知msg

## 参数

| 参数名             | 解释                                  | 是否必须             | 数值类型 | 默认值                 |
| ------------------ | ------------------------------------- | -------------------- | ------------------- | ----------------------------------------------------------------------- |
| cache_len_limit          | 设置缓存的图片buffer长度            | 否                   | int | 8                   |                                                                         |
| feed_type          | 图片来源, 0：本地；1：订阅            | 否                   | int | 0                   |                                                                         |
| image              | 本地图片地址                          | 否                   | string | config/4.jpg     |                                                                         |
| encoder_model_file_name              | 编码模型                          | 否                   | string | config/edgesam_encoder_1024.bin     |                                                                         |
| decoder_model_file_name              | 解码模型                          | 否                   | string | config/edgesam_decoder_1024.bin     |                                                                         |
| is_sync_mode  | 0: 同步推理, 1: 异步推理        | 否                   | int | 0                   |                                                                         |
| is_shared_mem_sub  | 使用shared mem通信方式订阅图片        | 否                   | int | 0                   |                                                                         |
| is_regular_box  | 使用固定检测框输入SAM        | 否                   | int | 0                   |                                                                         |
| is_padding_seg  | 是否对分割结果padding适配双目图        | 否                   | int | 0                   |                                                                         |
| dump_render_img    | 是否进行渲染，0：否；1：是            | 否                   | int | 0                   |                                                                         |
| ai_msg_sub_topic_name | 订阅上游检测结果的topicname,用于SAM输入 | 否                   | string | /hobot_dnn_detection | |
| ai_msg_pub_topic_name | 发布智能结果的topicname,用于web端展示 | 否                   | string | /perception/segmentation/edgesam | |
| ros_img_sub_topic_name | 接收ros图片话题名 | 否                   | string | /image | |

## 使用说明

- 控制话题：mono_edgesam 支持通过ai msg话题消息获取目标检测框。使用示例：
```shell
ros2 topic pub /hobot_dnn_detection ai_msgs/msg/PerceptionTargets '{"targets": [{"rois": [{"rect": {"x_offset": 96, "y_offset": 96, "width": 192, "height": 96}, "type": "anything"}]}] }'
```

## 运行

- mono_edgesam 使用到的模型在安装包'config'路径下。

- 编译成功后, 将生成的install路径拷贝到地平线RDK上（如果是在RDK上编译, 忽略拷贝步骤）, 并执行如下命令运行。

## X5 Ubuntu系统上运行

运行方式1, 使用可执行文件启动：
```shell
export COLCON_CURRENT_PREFIX=./install
source ./install/local_setup.bash
# config中为示例使用的模型, 回灌使用的本地图片
# 根据实际安装路径进行拷贝（docker中的安装路径为install/lib/mono_edgesam/config/, 拷贝命令为cp -r install/lib/mono_edgesam/config/ .）。
cp -r install/lib/mono_edgesam/config/ .

# 运行模式1：
# 使用本地jpg格式图片进行回灌预测
ros2 run mono_edgesam mono_edgesam --ros-args -p feed_type:=0 -p image:=config/4.jpg -p image_type:=0 -p dump_render_img:=1

# 运行模式2：使用shared mem通信方式(topic为/hbmem_img)进行预测,使用固定检测框进行SAM检测
ros2 run mono_edgesam mono_edgesam --ros-args -p feed_type:=1 -p is_shared_mem_sub:=1 -p is_regular_box:=1 --ros-args --log-level warn

# 运行模式3：
# 使用shared mem通信方式(topic为/hbmem_img)进行预测, 设置ai订阅话题名(/hobot_dnn_detection)为并设置log级别为warn。同时在另一个窗口发送ai msg话题(topic为/hobot_dnn_detection) 变更检测框
ros2 run mono_edgesam mono_edgesam --ros-args -p feed_type:=1 --ros-args --log-level warn -p ai_msg_sub_topic_name:="/hobot_dnn_detection"

ros2 topic pub /hobot_dnn_detection ai_msgs/msg/PerceptionTargets '{"targets": [{"rois": [{"rect": {"x_offset": 96, "y_offset": 96, "width": 192, "height": 96}, "type": "anything"}]}] }'

```

运行方式2, 使用launch文件启动：
```shell
export COLCON_CURRENT_PREFIX=./install
source ./install/setup.bash
# config中为示例使用的模型, 根据实际安装路径进行拷贝
# 如果是板端编译（无--merge-install编译选项）, 拷贝命令为cp -r install/PKG_NAME/lib/PKG_NAME/config/ ., 其中PKG_NAME为具体的package名。
cp -r install/lib/mono_edgesam/config/ .

# 配置MIPI摄像头
export CAM_TYPE=mipi

# 运行模式1：启动launch文件, 单独启动 sam 节点
ros2 launch mono_edgesam sam.launch.py

# 运行模式2：启动launch文件, 启动检测节点 + sam节点
ros2 launch mono_edgesam sam_with_dosod.launch.py
```

## X5 yocto系统上运行

```shell
export ROS_LOG_DIR=/userdata/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./install/lib/

# config中为示例使用的模型, 回灌使用的本地图片
cp -r install/lib/mono_edgesam/config/ .

# 运行模式1：
# 使用本地jpg格式图片进行回灌预测, 输入自定义类别
./install/lib/mono_edgesam/mono_edgesam --ros-args -p feed_type:=0 -p image:=config/4.jpg -p image_type:=0 -p dump_render_img:=1

# 运行模式2：使用shared mem通信方式(topic为/hbmem_img)进行预测,使用固定检测框进行SAM检测
./install/lib/mono_edgesam/mono_edgesam --ros-args -p feed_type:=1 -p is_shared_mem_sub:=1 -p is_regular_box:=1 --ros-args --log-level warn

# 运行模式3：
# 使用订阅到的image msg(topic为/image)进行预测, 设置ai订阅话题名(/hobot_dnn_detection)为并设置log级别为warn。同时在另一个窗口发送ai msg话题(topic为/hobot_dnn_detection) 变更检测框
./install/lib/mono_edgesam/mono_edgesam --ros-args -p feed_type:=1 --ros-args --log-level warn -p ai_msg_sub_topic_name:="/hobot_dnn_detection"

ros2 topic pub /hobot_dnn_detection ai_msgs/msg/PerceptionTargets '{"targets": [{"rois": [{"rect": {"x_offset": 96, "y_offset": 96, "width": 192, "height": 96}, "type": "anything"}]}] }'
```

## X86 Ubuntu系统上运行

```shell
export COLCON_CURRENT_PREFIX=./install
source ./install/setup.bash
# config中为示例使用的模型, 根据实际安装路径进行拷贝
cp -r ./install/lib/mono_edgesam/config/ .

export CAM_TYPE=fb

# 运行模式1：启动launch文件, 单独启动 sam 节点
ros2 launch mono_edgesam sam.launch.py

# 运行模式2：启动launch文件, 启动检测节点 + sam节点
ros2 launch mono_edgesam sam_with_dosod.launch.py
```

# 结果分析

## X5结果展示

log：

运行命令：`ros2 run mono_edgesam mono_edgesam --ros-args -p feed_type:=0 -p image:=config/4.jpg -p dump_render_img:=1`

```shell
[WARN] [1752828713.164111693] [mono_edgesam]: Parameter:
 cache_len_limit: 8
 dump_render_img: 1
 feed_type(0:local, 1:sub): 0
 image: config/4.jpg
 encoder_model_file_name: config/edgesam_encoder_1024.bin
 decoder_model_file_name: config/edgesam_decoder_1024.bin
 is_regular_box: 0
 is_padding_seg: 0
 is_shared_mem_sub: 1
 is_sync_mode: 0
 ai_msg_pub_topic_name: /perception/segmentation/edgesam
 ai_msg_sub_topic_name: /hobot_dnn_detection
 ros_img_sub_topic_name: /image
[BPU_PLAT]BPU Platform Version(1.3.6)!
[HBRT] set log level as 0. version = 3.15.55.0
[DNN] Runtime version = 1.24.5_(3.15.55 HBRT)
[A][DNN][packed_model.cpp:247][Model](2025-06-16,10:46:58.290.296) [HorizonRT] The model builder version = 1.24.3
[A][DNN][packed_model.cpp:247][Model](2025-06-16,10:46:58.561.595) [HorizonRT] The model builder version = 1.24.3
[INFO] [1750042018.628375694] [mono_edgesam]:
Model Info:
name: edgesam_encoder.
[input]
 - (0) Layout: NCHW, Shape: [1, 3, 1024, 1024], Type: HB_DNN_IMG_TYPE_NV12.
[output]
 - (0) Layout: NCHW, Shape: [1, 256, 64, 64], Type: HB_DNN_TENSOR_TYPE_F32.

Model Info:
name: edgesam_decoder.
[input]
 - (0) Layout: NCHW, Shape: [1, 1, 4, 1], Type: HB_DNN_TENSOR_TYPE_F32.
 - (1) Layout: NCHW, Shape: [1, 256, 64, 64], Type: HB_DNN_TENSOR_TYPE_F32.
[output]
 - (0) Layout: NONE, Shape: [1, 1, 1, 4], Type: HB_DNN_TENSOR_TYPE_S16.
 - (1) Layout: NCHW, Shape: [1, 4, 256, 256], Type: HB_DNN_TENSOR_TYPE_S8.

[INFO] [1752828713.949130093] [mono_edgesam]: seg result size: 720896 valid_h: 704 valid_w: 1024
[INFO] [1752828714.000021486] [sam ouput parser]: Draw result to file: render_sam_feedback_0_0.jpeg
```

## 渲染结果
![image](img/render_sam_feedback_0_0.jpeg)

说明：前处理对图片进行缩放和补全处理。