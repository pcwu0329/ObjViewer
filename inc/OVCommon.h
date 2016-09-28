#pragma once

#include <Eigen/Dense>
#include <stdint.h>

//#define VERBOSE

namespace ov
{

typedef unsigned char byte;

typedef Eigen::VectorXd Vec;
typedef Eigen::MatrixXd Mat;

typedef Eigen::Vector2d Vec2;
typedef Eigen::Vector3d Vec3;
typedef Eigen::Vector4d Vec4;
typedef Eigen::Matrix<double, 6, 1> Vec6;
typedef Eigen::Matrix<double, 7, 1> Vec7;

typedef Eigen::Matrix<double, 3, 3> Mat3;
typedef Eigen::Matrix<double, 4, 4> Mat4;
typedef Eigen::Matrix<double, 6, 6> Mat6;
typedef Eigen::Matrix<double, 7, 7> Mat7;
typedef Eigen::Matrix<double, 3, 4> Mat34;
typedef Eigen::Matrix<double, 1, Eigen::Dynamic> Mat1X;
typedef Eigen::Matrix<double, 2, Eigen::Dynamic> Mat2X;
typedef Eigen::Matrix<double, 3, Eigen::Dynamic> Mat3X;
typedef Eigen::Matrix<double, 4, Eigen::Dynamic> Mat4X;
typedef Eigen::Matrix<double, 6, Eigen::Dynamic> Mat6X;
typedef Eigen::Matrix<double, 7, Eigen::Dynamic> Mat7X;
typedef Eigen::Matrix<double, Eigen::Dynamic, 1> MatX1;
typedef Eigen::Matrix<double, Eigen::Dynamic, 2> MatX2;
typedef Eigen::Matrix<double, Eigen::Dynamic, 3> MatX3;
typedef Eigen::Matrix<double, Eigen::Dynamic, 4> MatX4;
typedef Eigen::Matrix<double, Eigen::Dynamic, 6> MatX6;
typedef Eigen::Matrix<double, Eigen::Dynamic, 7> MatX7;

typedef Eigen::Array<bool, Eigen::Dynamic, 1> Mask;

typedef Eigen::VectorXf Vecf;
typedef Eigen::MatrixXf Matf;
typedef Eigen::Vector2f Vec2f;
typedef Eigen::Vector3f Vec3f;
typedef Eigen::Vector4f Vec4f;
typedef Eigen::Matrix<float, 2, Eigen::Dynamic> Mat2Xf;
typedef Eigen::Matrix<float, 3, Eigen::Dynamic> Mat3Xf;
typedef Eigen::Matrix<float, 4, Eigen::Dynamic> Mat4Xf;

typedef Eigen::Array<uint8_t, Eigen::Dynamic, Eigen::Dynamic> ArrayXXu8;
typedef Eigen::Array<uint16_t, Eigen::Dynamic, Eigen::Dynamic> ArrayXXu16;
// ArrayXXf and ArrayXXd are already defined in eigen.

}