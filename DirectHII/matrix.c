/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "precompiled.h"

#include "matrix.h"
#include "mathlib.h"

#include <xmmintrin.h>
#include <smmintrin.h>

/*
============================================================================================================

MATRIX OPS

These happen in pace on the matrix and update it's current values

============================================================================================================
*/

QMATRIX *R_MatrixIdentity (QMATRIX *m)
{
	return R_MatrixLoadf (m, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}


QMATRIX *R_MatrixMultiply (QMATRIX *out, QMATRIX *m1, QMATRIX *m2)
{
	// https://github.com/mhQuake/DirectQII/issues/1
	int i;

	__m128 row1 = _mm_load_ps (m2->m4x4[0]);
	__m128 row2 = _mm_load_ps (m2->m4x4[1]);
	__m128 row3 = _mm_load_ps (m2->m4x4[2]);
	__m128 row4 = _mm_load_ps (m2->m4x4[3]);

	for (i = 0; i < 4; i++)
	{
		__m128 brod1 = _mm_set1_ps (m1->m4x4[i][0]);
		__m128 brod2 = _mm_set1_ps (m1->m4x4[i][1]);
		__m128 brod3 = _mm_set1_ps (m1->m4x4[i][2]);
		__m128 brod4 = _mm_set1_ps (m1->m4x4[i][3]);

		__m128 row = _mm_add_ps (
			_mm_add_ps (
			_mm_mul_ps (brod1, row1),
			_mm_mul_ps (brod2, row2)
			),
			_mm_add_ps (
			_mm_mul_ps (brod3, row3),
			_mm_mul_ps (brod4, row4)
			)
			);

		_mm_store_ps (out->m4x4[i], row);
	}

	return out;
}


QMATRIX *R_MatrixOrtho (QMATRIX *m, float left, float right, float bottom, float top, float zNear, float zFar)
{
	QMATRIX m1 = {
		2 / (right - left),
		0,
		0,
		0,
		0,
		2 / (top - bottom),
		0,
		0,
		0,
		0,
		-2 / (zFar - zNear),
		0,
		-((right + left) / (right - left)),
		-((top + bottom) / (top - bottom)),
		-((zFar + zNear) / (zFar - zNear)),
		1
	};

	return R_MatrixMultiply (m, &m1, m);
}


QMATRIX *R_MatrixFrustum (QMATRIX *m, float fovx, float fovy, float zn, float zf)
{
	float r = zn * tan ((fovx * M_PI) / 360.0);
	float t = zn * tan ((fovy * M_PI) / 360.0);

	float l = -r;
	float b = -t;

	// infinite projection variant without epsilon for shadows but with adjusting for LH to RH
	// http://www.geometry.caltech.edu/pubs/UD12.pdf
	QMATRIX m2 = {
		2 * zn / (r - l),
		0,
		0,
		0,
		0,
		2 * zn / (t - b),
		0,
		0,
		(l + r) / (r - l),
		(t + b) / (t - b),
		-1, //zf / (zn - zf),
		-1,
		0,
		0,
		-zn, //zn * zf / (zn - zf),
		0
	};

	return R_MatrixMultiply (m, &m2, m);
}


QMATRIX *R_MatrixLoadf (QMATRIX *m, float _11, float _12, float _13, float _14, float _21, float _22, float _23, float _24, float _31, float _32, float _33, float _34, float _41, float _42, float _43, float _44)
{
	m->m4x4[0][0] = _11; m->m4x4[0][1] = _12; m->m4x4[0][2] = _13; m->m4x4[0][3] = _14;
	m->m4x4[1][0] = _21; m->m4x4[1][1] = _22; m->m4x4[1][2] = _23; m->m4x4[1][3] = _24;
	m->m4x4[2][0] = _31; m->m4x4[2][1] = _32; m->m4x4[2][2] = _33; m->m4x4[2][3] = _34;
	m->m4x4[3][0] = _41; m->m4x4[3][1] = _42; m->m4x4[3][2] = _43; m->m4x4[3][3] = _44;

	return m;
}


QMATRIX *R_MatrixLoad (QMATRIX *dst, QMATRIX *src)
{
	memcpy (dst, src, sizeof (QMATRIX));
	return dst;
}


QMATRIX *R_MatrixTranslate (QMATRIX *m, float x, float y, float z)
{
	m->m4x4[3][0] += x * m->m4x4[0][0] + y * m->m4x4[1][0] + z * m->m4x4[2][0];
	m->m4x4[3][1] += x * m->m4x4[0][1] + y * m->m4x4[1][1] + z * m->m4x4[2][1];
	m->m4x4[3][2] += x * m->m4x4[0][2] + y * m->m4x4[1][2] + z * m->m4x4[2][2];
	m->m4x4[3][3] += x * m->m4x4[0][3] + y * m->m4x4[1][3] + z * m->m4x4[2][3];

	return m;
}


QMATRIX *R_MatrixScale (QMATRIX *m, float x, float y, float z)
{
	Vector4Scalef (m->m4x4[0], m->m4x4[0], x);
	Vector4Scalef (m->m4x4[1], m->m4x4[1], y);
	Vector4Scalef (m->m4x4[2], m->m4x4[2], z);

	return m;
}


QMATRIX *R_MatrixRotate (QMATRIX *m, float p, float y, float r)
{
	float sr = sin (DEG2RAD (r));
	float sp = sin (DEG2RAD (p));
	float sy = sin (DEG2RAD (y));
	float cr = cos (DEG2RAD (r));
	float cp = cos (DEG2RAD (p));
	float cy = cos (DEG2RAD (y));

	QMATRIX m1 = {
		(cp * cy),
		(cp * sy),
		-sp,
		0.0f,
		(cr * -sy) + (sr * sp * cy),
		(cr * cy) + (sr * sp * sy),
		(sr * cp),
		0.0f,
		(sr * sy) + (cr * sp * cy),
		(-sr * cy) + (cr * sp * sy),
		(cr * cp),
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};

	return R_MatrixMultiply (m, &m1, m);
}


QMATRIX *R_MatrixCamera (QMATRIX *m, const float *origin, const float *angles)
{
	float sr = sin (DEG2RAD (angles[2]));
	float sp = sin (DEG2RAD (angles[0]));
	float sy = sin (DEG2RAD (angles[1]));
	float cr = cos (DEG2RAD (angles[2]));
	float cp = cos (DEG2RAD (angles[0]));
	float cy = cos (DEG2RAD (angles[1]));

	float _11 = -((cr * -sy) + (sr * sp * cy));
	float _21 = -((cr * cy) + (sr * sp * sy));
	float _31 = -(sr * cp);

	float _12 = (sr * sy) + (cr * sp * cy);
	float _22 = (-sr * cy) + (cr * sp * sy);
	float _32 = (cr * cp);

	float _13 = -(cp * cy);
	float _23 = -(cp * sy);
	float _33 = sp;

	QMATRIX m2 = {
		_11, _12, _13,
		0.0f,
		_21, _22, _23,
		0.0f,
		_31, _32, _33,
		0.0f,
		-origin[0] * _11 - origin[1] * _21 - origin[2] * _31,
		-origin[0] * _12 - origin[1] * _22 - origin[2] * _32,
		-origin[0] * _13 - origin[1] * _23 - origin[2] * _33,
		1.0f
	};

	return R_MatrixMultiply (m, &m2, m);
}


float *R_VectorTransform (QMATRIX *m, float *out, float *in)
{
	out[0] = in[0] * m->m4x4[0][0] + in[1] * m->m4x4[1][0] + in[2] * m->m4x4[2][0] + m->m4x4[3][0];
	out[1] = in[1] * m->m4x4[0][1] + in[1] * m->m4x4[1][1] + in[2] * m->m4x4[2][1] + m->m4x4[3][1];
	out[2] = in[2] * m->m4x4[0][2] + in[1] * m->m4x4[1][2] + in[2] * m->m4x4[2][2] + m->m4x4[3][2];

	return out;
}


float *R_VectorInverseTransform (QMATRIX *m, float *out, float *in)
{
	// http://content.gpwiki.org/index.php/MathGem:Fast_Matrix_Inversion
	out[0] = in[0] * m->m4x4[0][0] + in[1] * m->m4x4[0][1] + in[2] * m->m4x4[0][2] - Vector3Dot (m->m4x4[0], m->m4x4[3]);
	out[1] = in[0] * m->m4x4[1][0] + in[1] * m->m4x4[1][1] + in[2] * m->m4x4[1][2] - Vector3Dot (m->m4x4[1], m->m4x4[3]);
	out[2] = in[0] * m->m4x4[2][0] + in[1] * m->m4x4[2][1] + in[2] * m->m4x4[2][2] - Vector3Dot (m->m4x4[2], m->m4x4[3]);

	return out;
}

