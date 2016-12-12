/*
*
* Copyright (C) 2016 OtherCrashOverride@users.noreply.github.com.
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2, as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
*/

#include <cmath>

#include "Matrix4.h"
#include "Exception.h"



const Matrix4 Matrix4::Identity = Matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1);

Matrix4 Matrix4::CreatePerspectiveFieldOfView(float fieldOfView, float aspectRatio, float nearPlaneDistance, float farPlaneDistance)
{
	// Validate
	if ((fieldOfView <= 0) || (fieldOfView >= (M_PI)))
		throw Exception("fieldOfView must be between 0 and PI.");

	if (nearPlaneDistance <= 0)
		throw Exception("nearPlaneDistance must be greater than zero.");

	if (farPlaneDistance <= 0 || nearPlaneDistance >= farPlaneDistance)
		throw Exception("farPlaneDistance must be greater than zero and nearPlaneDistance.");



	// Based on Sony Vector Math Libray
	float f;
	float rangeInv;

	const float piOverTwo = M_PI_2;

	f = tan(piOverTwo - (0.5f * fieldOfView));
	rangeInv = (1.0f / (nearPlaneDistance - farPlaneDistance));


	Matrix4 result;

	result.M11 = f / aspectRatio;
	result.M12 = 0;
	result.M13 = 0;
	result.M14 = 0;

	result.M21 = 0;
	result.M22 = f;
	result.M23 = 0;
	result.M24 = 0;

	result.M31 = 0;
	result.M32 = 0;
	result.M33 = ((nearPlaneDistance + farPlaneDistance) * rangeInv);
	result.M34 = -1.0f;

	result.M41 = 0;
	result.M42 = 0;
	result.M43 = (((nearPlaneDistance * farPlaneDistance) * rangeInv) * 2.0f);
	result.M44 = 1;

	return result;
}

Matrix4 Matrix4::CreateOrthographicOffCenter(float left, float top, float right, float bottom, float zNearPlane, float zFarPlane)
{
	// From Monogame
	Matrix4 result;

	result.M11 = (float)(2.0 / ((double)right - (double)left));
	result.M12 = 0.0f;
	result.M13 = 0.0f;
	result.M14 = 0.0f;
	result.M21 = 0.0f;
	result.M22 = (float)(2.0 / ((double)top - (double)bottom));
	result.M23 = 0.0f;
	result.M24 = 0.0f;
	result.M31 = 0.0f;
	result.M32 = 0.0f;
	result.M33 = (float)(1.0 / ((double)zNearPlane - (double)zFarPlane));
	result.M34 = 0.0f;
	result.M41 = (float)(((double)left + (double)right) / ((double)left - (double)right));
	result.M42 = (float)(((double)top + (double)bottom) / ((double)bottom - (double)top));
	result.M43 = (float)((double)zNearPlane / ((double)zNearPlane - (double)zFarPlane));
	result.M44 = 1.0f;

	return result;
}

Matrix4 Matrix4::CreateTranspose(const Matrix4& matrix)
{
	Matrix4 result(matrix.M11, matrix.M21, matrix.M31, matrix.M41,
		matrix.M12, matrix.M22, matrix.M32, matrix.M42,
		matrix.M13, matrix.M23, matrix.M33, matrix.M43,
		matrix.M14, matrix.M24, matrix.M34, matrix.M44);


	return result;
}

Matrix4 Matrix4::CreateLookAt(const Vector3& cameraPosition, const Vector3& cameraTarget, const Vector3& cameraUpVector)
{

	Vector3 forward = cameraPosition - cameraTarget;
	forward.Normalize();

	Vector3 normalizeCameraUp = cameraUpVector;
	normalizeCameraUp.Normalize();


	// from http://www.fastgraph.com/makegames/3drotation/
	// "The Mathematics of the 3D Rotation Matrix"
	float d = Vector3::Dot(normalizeCameraUp, forward);
	Vector3 up = normalizeCameraUp - (forward * d);


	Vector3 right = Vector3::Cross(up, forward);
	right.Normalize();


	// Create the matrix
	Matrix4 result;


	result.M11 = right.X;
	result.M12 = up.X;
	result.M13 = forward.X;
	result.M14 = 0;

	result.M21 = right.Y;
	result.M22 = up.Y;
	result.M23 = forward.Y;
	result.M24 = 0;

	result.M31 = right.Z;
	result.M32 = up.Z;
	result.M33 = forward.Z;
	result.M34 = 0;

	result.M41 = -Vector3::Dot(right, cameraPosition);
	result.M42 = -Vector3::Dot(up, cameraPosition);
	result.M43 = -Vector3::Dot(forward, cameraPosition);
	result.M44 = 1;

	return result;
}

Matrix4 Matrix4::CreateRotationX(float radians)
{
	float s = sin(radians);
	float c = cos(radians);

	return Matrix4(1, 0, 0, 0,
		0, c, s, 0,
		0, -s, c, 0,
		0, 0, 0, 1);
}

Matrix4 Matrix4::CreateRotationY(float radians)
{
	float s = sin(radians);
	float c = cos(radians);

	return Matrix4(c, 0, -s, 0,
		0, 1, 0, 0,
		s, 0, c, 0,
		0, 0, 0, 1);
}

Matrix4 Matrix4::CreateRotationZ(float radians)
{
	float s = sin(radians);
	float c = cos(radians);

	return Matrix4(c, s, 0, 0,
		-s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
}



Matrix4& Matrix4::operator*=(const Matrix4& rhs)
{
	float m11 = (((M11 * rhs.M11) + (M12 * rhs.M21)) + (M13 * rhs.M31)) + (M14 * rhs.M41);
	float m12 = (((M11 * rhs.M12) + (M12 * rhs.M22)) + (M13 * rhs.M32)) + (M14 * rhs.M42);
	float m13 = (((M11 * rhs.M13) + (M12 * rhs.M23)) + (M13 * rhs.M33)) + (M14 * rhs.M43);
	float m14 = (((M11 * rhs.M14) + (M12 * rhs.M24)) + (M13 * rhs.M34)) + (M14 * rhs.M44);
	float m21 = (((M21 * rhs.M11) + (M22 * rhs.M21)) + (M23 * rhs.M31)) + (M24 * rhs.M41);
	float m22 = (((M21 * rhs.M12) + (M22 * rhs.M22)) + (M23 * rhs.M32)) + (M24 * rhs.M42);
	float m23 = (((M21 * rhs.M13) + (M22 * rhs.M23)) + (M23 * rhs.M33)) + (M24 * rhs.M43);
	float m24 = (((M21 * rhs.M14) + (M22 * rhs.M24)) + (M23 * rhs.M34)) + (M24 * rhs.M44);
	float m31 = (((M31 * rhs.M11) + (M32 * rhs.M21)) + (M33 * rhs.M31)) + (M34 * rhs.M41);
	float m32 = (((M31 * rhs.M12) + (M32 * rhs.M22)) + (M33 * rhs.M32)) + (M34 * rhs.M42);
	float m33 = (((M31 * rhs.M13) + (M32 * rhs.M23)) + (M33 * rhs.M33)) + (M34 * rhs.M43);
	float m34 = (((M31 * rhs.M14) + (M32 * rhs.M24)) + (M33 * rhs.M34)) + (M34 * rhs.M44);
	float m41 = (((M41 * rhs.M11) + (M42 * rhs.M21)) + (M43 * rhs.M31)) + (M44 * rhs.M41);
	float m42 = (((M41 * rhs.M12) + (M42 * rhs.M22)) + (M43 * rhs.M32)) + (M44 * rhs.M42);
	float m43 = (((M41 * rhs.M13) + (M42 * rhs.M23)) + (M43 * rhs.M33)) + (M44 * rhs.M43);
	float m44 = (((M41 * rhs.M14) + (M42 * rhs.M24)) + (M43 * rhs.M34)) + (M44 * rhs.M44);

	M11 = m11;
	M12 = m12;
	M13 = m13;
	M14 = m14;
	M21 = m21;
	M22 = m22;
	M23 = m23;
	M24 = m24;
	M31 = m31;
	M32 = m32;
	M33 = m33;
	M34 = m34;
	M41 = m41;
	M42 = m42;
	M43 = m43;
	M44 = m44;
	return *this;
}
const Matrix4 Matrix4::operator*(const Matrix4 &other) const
{
	Matrix4 result(*this);
	result *= other;
	return result;
}
