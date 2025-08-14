// Copyright (c) 2025，D-Robotics.
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

std::shared_ptr<DNNTensor> InputPreProcessor::GetYTensorFromNV12Img(
                                          const char *in_img_data,
                                          const int &in_img_height,
                                          const int &in_img_width,
                                          int scaled_img_height,
                                          int scaled_img_width,
                                          hbDNNTensorProperties &tensor_properties) {

  auto *y = new hbUCPSysMem;
  auto w_stride = ALIGN_16(scaled_img_width);

  hbUCPMallocCached(y, scaled_img_height * w_stride, 0);
  const uint8_t *data = reinterpret_cast<const uint8_t *>(in_img_data);
  auto *hb_mem_addr = reinterpret_cast<uint8_t *>(y->virAddr);
  memset(y->virAddr, 0, scaled_img_height * w_stride);

  int copy_w = std::min(in_img_width, scaled_img_width);
  int copy_h = std::min(in_img_height, scaled_img_height);

  // padding y
  for (int h = 0; h < copy_h; ++h) {
    auto *raw = hb_mem_addr + h * w_stride;
    auto *src = data + h * in_img_width;
    memcpy(raw, src, copy_w);
  }

  hbUCPMemFlush(y, HB_SYS_MEM_CACHE_CLEAN);
  auto input_tensor = new DNNTensor;
  input_tensor->properties = tensor_properties;
  input_tensor->properties.alignedByteSize = scaled_img_height * scaled_img_width;
  input_tensor->properties.stride[1] = input_tensor->properties.stride[2] * input_tensor->properties.validShape.dimensionSize[2];
  input_tensor->properties.stride[0] = input_tensor->properties.stride[1] * input_tensor->properties.validShape.dimensionSize[1];
  
  input_tensor->sysMem.virAddr = reinterpret_cast<void *>(y->virAddr);
  input_tensor->sysMem.phyAddr = y->phyAddr;
  input_tensor->sysMem.memSize = scaled_img_height * w_stride;

  return std::shared_ptr<DNNTensor>(
      input_tensor, [y](DNNTensor *input_tensor) {
        // Release memory after deletion
        hbUCPFree(y);
        delete y;
        delete input_tensor;
      });
}

std::shared_ptr<DNNTensor> InputPreProcessor::GetUVTensorFromNV12Img(
                                          const char *in_img_data,
                                          const int &in_img_height,
                                          const int &in_img_width,
                                          int scaled_img_height,
                                          int scaled_img_width,
                                          hbDNNTensorProperties &tensor_properties) {

  auto *uv = new hbUCPSysMem;
  auto w_stride = ALIGN_16(scaled_img_width);
  hbUCPMallocCached(uv, scaled_img_height * w_stride / 2, 0);
  const uint8_t *data = reinterpret_cast<const uint8_t *>(in_img_data);
  auto *hb_mem_addr = reinterpret_cast<uint8_t *>(uv->virAddr);
  memset(uv->virAddr, 0, scaled_img_height * w_stride / 2);

  int copy_w = std::min(in_img_width, scaled_img_width);
  int copy_h = std::min(in_img_height, scaled_img_height);

  // padding uv
  auto uv_data = in_img_data + in_img_height * in_img_width;
  for (int32_t h = 0; h < copy_h / 2; ++h) {
    auto *raw = hb_mem_addr + h * w_stride;
    auto *src = uv_data + h * in_img_width;
    memcpy(raw, src, copy_w);
  }

  hbUCPMemFlush(uv, HB_SYS_MEM_CACHE_CLEAN);
  auto input_tensor = new DNNTensor;
  input_tensor->properties = tensor_properties;
  input_tensor->properties.alignedByteSize = scaled_img_height * scaled_img_width / 2;
  input_tensor->properties.stride[1] = input_tensor->properties.stride[2] * input_tensor->properties.validShape.dimensionSize[2];
  input_tensor->properties.stride[0] = input_tensor->properties.stride[1] * input_tensor->properties.validShape.dimensionSize[1];
  input_tensor->sysMem.virAddr = reinterpret_cast<void *>(uv->virAddr);
  input_tensor->sysMem.phyAddr = uv->phyAddr;
  input_tensor->sysMem.memSize = scaled_img_height * scaled_img_width / 2;

  return std::shared_ptr<DNNTensor>(
      input_tensor, [uv](DNNTensor *input_tensor) {
        // Release memory after deletion
        hbUCPFree(uv);
        delete uv;
        delete input_tensor;
      });
}

std::shared_ptr<DNNTensor> InputPreProcessor::GetBoxTensor(
    const std::vector<std::vector<float>> & boxes,
    hbDNNTensorProperties tensor_properties) {

  int src_elem_size = 4;

  int num_boxes = tensor_properties.validShape.dimensionSize[1];
  auto *uv = new hbUCPSysMem;
  hbUCPMallocCached(uv, num_boxes * 4 * src_elem_size, 0);
  //内存初始化
  memset(uv->virAddr, 0, num_boxes * 4 * src_elem_size);
  auto *hb_mem_addr = reinterpret_cast<uint8_t *>(uv->virAddr);

  for (auto &box: boxes) {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(box.data());
    memcpy(hb_mem_addr, data, 4 * src_elem_size);
    hb_mem_addr += 4 * src_elem_size;
  }

  hbUCPMemFlush(uv, HB_SYS_MEM_CACHE_CLEAN);
  auto input_tensor = new DNNTensor;

  input_tensor->properties = tensor_properties;
  input_tensor->sysMem.virAddr = reinterpret_cast<void *>(uv->virAddr);
  input_tensor->sysMem.phyAddr = uv->phyAddr;
  input_tensor->sysMem.memSize = num_boxes * 4 * src_elem_size;

  return std::shared_ptr<DNNTensor>(
      input_tensor, [uv](DNNTensor *input_tensor) {
        // Release memory after deletion
        hbUCPFree(uv);
        delete uv;
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