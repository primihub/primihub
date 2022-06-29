
#ifndef EIGEN_H
#define EIGEN_H

#pragma once
#include "globals.h"
#include "Eigen/Dense"
namespace primihub
{
	namespace falcon
	{
		using namespace Eigen;
		using eig_mat = Matrix<myType, Dynamic, Dynamic>;

		class eigenMatrix
		{
		public:
			std::array<eig_mat, 2> m_share;
			size_t rows;
			size_t columns;

			eigenMatrix(size_t _rows, size_t _columns)
				: m_share({eig_mat(_rows, _columns), eig_mat(_rows, _columns)}),
				  rows(_rows),
				  columns(_columns) {}

			eigenMatrix operator*(const eigenMatrix &B)
			{
				assert(this->columns == B.rows && "Dimension mismatch");
				eigenMatrix ret(this->rows, B.columns);
				ret.m_share[0] = this->m_share[0] * B.m_share[0] + this->m_share[1] * B.m_share[0] + this->m_share[0] * B.m_share[1];
				ret.m_share[1] = eig_mat::Zero(this->rows, B.columns);
				return ret;
			}
		};

	}
}
#endif
