#pragma once

#include <GL/gl.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <unordered_map>
#include "TinyObjLoader.h"

namespace ov
{

bool
LoadTextures(std::vector<tinyobj::material_t>& materials,
             std::unordered_map<std::string, GLuint>& textureIds,
             const std::string& dir);

bool
LoadTexture(cv::Mat& texture, const std::string& filename);

bool
LoadTGA(cv::Mat& texture, const std::string& filename);

} // namespace ov
