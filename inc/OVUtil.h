#pragma once

#include <Eigen/Dense>
#include <opencv2/core/eigen.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <AtlBase.h>
#include "ObjViewer.h"
#include "OVCommon.h"

namespace ov
{

#define TRACKBALLSIZE 0.8

#ifndef GL_BGR
#define GL_BGR GL_BGR_EXT
#endif

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif

std::string
GetFileName(const std::string& s);

std::string
GetBaseName(const std::string& s);

std::string
GetExt(const std::string& s);

std::string
GetDir(const std::string& s);

Mat3
Trackball(const Vec2& prePos, const Vec2& curPos);

double
GetZValueFrom2DPoint(double r, const Vec2& pos);

Vec3
FromRotationMatirxToAxisAngle(const Mat3& R);

Mat3
FromAxisAngleToRotationMatrix(const Vec3& r);

wxCheckBox*
CreateCheckBoxAndAddToSizer(wxWindow* parent,
                            wxSizer *sizer,
                            wxString labelStr,
                            wxWindowID id);

Mat
LoadMatrix(std::string fileName);

std::string
ZeroPadNumber(int num, int width);

bool
IsDirectoryExists(std::string dirName);

void
CreateDirectorys(std::string path);

} // namespace ov
