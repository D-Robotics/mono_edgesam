// Copyright (c) 2024，D-Robotics.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fstream>
#include <vector>
#include <cstring> // for memcpy

#include "include/data_preprocess.h"

std::shared_ptr<DNNTensor> InputPreProcessor::GetBGRTensorFromBGR(
                                          const cv::Mat &bgr_mat_tmp,                                                     
                                          int scaled_img_height,
                                          int scaled_img_width,
                                          hbDNNTensorProperties &tensor_properties,
                                          float &dst_ratio) {
  cv::Mat bgr_mat;
  bgr_mat_tmp.copyTo(bgr_mat);
  cv::Mat pixel_values_mat;
  auto w_stride = ALIGN_16(scaled_img_width);
  int channel = 3;
  int src_elem_size = 4;
  int original_img_width = bgr_mat.cols;
  int original_img_height = bgr_mat.rows;

  cv::Mat pad_frame;
  pad_frame = cv::Mat(scaled_img_height, w_stride, CV_8UC3, cv::Scalar::all(0));
  bgr_mat.copyTo(pad_frame(cv::Rect(0, 0, bgr_mat.cols, bgr_mat.rows)));

  cv::Mat mat_tmp;
  pad_frame.convertTo(mat_tmp, CV_32F); 
  cv::cvtColor(mat_tmp, mat_tmp, cv::COLOR_BGR2RGB);
  cv::Scalar mean(123.675, 116.28, 103.53);  // BGR 通道均值
  cv::Scalar std(58.395, 57.12, 57.375);   // BGR 通道标准差

  // 按通道减去均值，再除以标准差
  std::vector<cv::Mat> channels(3);
  cv::split(mat_tmp, channels);  // 分离通道

  for (int i = 0; i < 3; i++) {
    channels[i] = (channels[i] - mean[i]) / std[i];
  }

  // 合并通道
  cv::merge(channels, pixel_values_mat);

  auto *mem = new hbSysMem;
  hbSysAllocCachedMem(mem, scaled_img_height * w_stride * channel * src_elem_size);
  uint8_t *data = pixel_values_mat.data;
  auto *hb_mem_addr = reinterpret_cast<uint8_t *>(mem->virAddr);

  if (tensor_properties.tensorLayout == HB_DNN_LAYOUT_NCHW) {
    for (int h = 0; h < scaled_img_height; ++h) {
      for (int w = 0; w < scaled_img_width; ++w) {
        for (int c = 0; c < channel; ++c) {
          auto *raw = hb_mem_addr + c * scaled_img_height * w_stride * src_elem_size + h * w_stride * src_elem_size + w * src_elem_size;
          auto *src = data + h * scaled_img_width * channel * src_elem_size + w * channel * src_elem_size + c * src_elem_size;
          memcpy(raw, src, src_elem_size);
        }
      }
    }
  } else {
    for (int h = 0; h < scaled_img_height; ++h) {
      auto *raw = hb_mem_addr + h * w_stride * channel * src_elem_size;
      auto *src = data + h * scaled_img_width * channel * src_elem_size;
      memcpy(raw, src, scaled_img_width * channel * src_elem_size);
    }
  }

  hbSysFlushMem(mem, HB_SYS_MEM_CACHE_CLEAN);
  auto input_tensor = new DNNTensor;
  input_tensor->properties = tensor_properties;
  input_tensor->sysMem[0].virAddr = reinterpret_cast<void *>(mem->virAddr);
  input_tensor->sysMem[0].phyAddr = mem->phyAddr;
  input_tensor->sysMem[0].memSize = scaled_img_height * scaled_img_width * channel * src_elem_size;

  return std::shared_ptr<DNNTensor>(
      input_tensor, [mem](DNNTensor *input_tensor) {
        // Release memory after deletion
        hbSysFreeMem(mem);
        delete mem;
        delete input_tensor;
      });
}

std::shared_ptr<DNNTensor> InputPreProcessor::GetNV12TensorFromNV12Img(
                                          const char *in_img_data,
                                          const int &in_img_height,
                                          const int &in_img_width,
                                          int scaled_img_height,
                                          int scaled_img_width,
                                          hbDNNTensorProperties &tensor_properties) {

  auto *mem = new hbSysMem;
  auto w_stride = ALIGN_16(scaled_img_width);

  hbSysAllocCachedMem(mem, scaled_img_height * w_stride * 3 / 2);
  const uint8_t *data = reinterpret_cast<const uint8_t *>(in_img_data);
  auto *hb_mem_addr = reinterpret_cast<uint8_t *>(mem->virAddr);
  memset(mem->virAddr, 0, scaled_img_height * w_stride * 3 / 2);

  int copy_w = std::min(in_img_width, scaled_img_width);
  int copy_h = std::min(in_img_height, scaled_img_height);

  // padding y
  for (int h = 0; h < copy_h; ++h) {
    auto *raw = hb_mem_addr + h * w_stride;
    auto *src = data + h * in_img_width;
    memcpy(raw, src, copy_w);
  }

  // padding uv
  auto uv_data = in_img_data + in_img_height * in_img_width;
  for (int32_t h = 0; h < copy_h / 2; ++h) {
    auto *raw = hb_mem_addr + scaled_img_height * w_stride + h * w_stride;
    auto *src = uv_data + h * in_img_width;
    memcpy(raw, src, copy_w);
  }

  hbSysFlushMem(mem, HB_SYS_MEM_CACHE_CLEAN);
  auto input_tensor = new DNNTensor;
  input_tensor->properties = tensor_properties;
  input_tensor->sysMem[0].virAddr = reinterpret_cast<void *>(mem->virAddr);
  input_tensor->sysMem[0].phyAddr = mem->phyAddr;
  input_tensor->sysMem[0].memSize = scaled_img_height * scaled_img_width * 3 / 2;

  return std::shared_ptr<DNNTensor>(
      input_tensor, [mem](DNNTensor *input_tensor) {
        // Release memory after deletion
        hbSysFreeMem(mem);
        delete mem;
        delete input_tensor;
      });
}

std::shared_ptr<DNNTensor> InputPreProcessor::GetBoxTensor(
    const std::vector<std::vector<float>> & boxes,
    hbDNNTensorProperties tensor_properties) {

  int src_elem_size = 4;

  int num_boxes = tensor_properties.alignedShape.dimensionSize[1];
  auto *mem = new hbSysMem;
  hbSysAllocCachedMem(mem, num_boxes * 4 * src_elem_size);
  //内存初始化
  memset(mem->virAddr, 0, num_boxes * 4 * src_elem_size);
  auto *hb_mem_addr = reinterpret_cast<uint8_t *>(mem->virAddr);

  for (auto &box: boxes) {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(box.data());
    memcpy(hb_mem_addr, data, 4 * src_elem_size);
    hb_mem_addr += 4 * src_elem_size;
  }

  hbSysFlushMem(mem, HB_SYS_MEM_CACHE_CLEAN);
  auto input_tensor = new DNNTensor;

  input_tensor->properties = tensor_properties;
  input_tensor->sysMem[0].virAddr = reinterpret_cast<void *>(mem->virAddr);
  input_tensor->sysMem[0].phyAddr = mem->phyAddr;
  input_tensor->sysMem[0].memSize = num_boxes * 4 * src_elem_size;

  return std::shared_ptr<DNNTensor>(
      input_tensor, [mem](DNNTensor *input_tensor) {
        // Release memory after deletion
        hbSysFreeMem(mem);
        delete mem;
        delete input_tensor;
      });
}


int GenScaleBox(std::shared_ptr<std::vector<hbDNNRoi>> &rois,
                                    std::vector<std::vector<float>> &boxes,
                                    float ratio) {
  for (auto it = rois->begin(); it != rois->end(); ++it) {
    // 访问每个hbDNNRoi实例
    hbDNNRoi roi = *it;
    std::vector<float> box;
    float x1 = static_cast<float>(roi.left / ratio);
    float y1 = static_cast<float>(roi.top / ratio);
    float x2 = static_cast<float>(roi.right / ratio);
    float y2 = static_cast<float>(roi.bottom / ratio);
    box.push_back(x1);
    box.push_back(y1);
    box.push_back(x2);
    box.push_back(y2);
    boxes.push_back(box);
  }
  return 0;
}