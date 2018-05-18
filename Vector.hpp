
#include "Intern.hpp"

//////////////////////////////////////////////////////////////////////////
//
// Fun with Graphics 
//
// Copyright (c) 2017 Granby Consulting LLC
//
// All Rights Reserved
//

#ifndef INCLUDE_VECTOR_HPP

#define INCLUDE_VECTOR_HPP

//////////////////////////////////////////////////////////////////////////
//
// Simple Vector
//

class CVector
{	
	public:
		double x, y;

		CVector const & operator += (CVector const &v)
		{
			x += v.x;
			y += v.y;

			return *this;
			}

		CVector operator + (CVector const &v) const
		{
			CVector r;

			r.x = x + v.x;
			r.y = y + v.y;

			return r;
			}

		CVector operator - (CVector const &v) const
		{
			CVector r;

			r.x = x - v.x;
			r.y = y - v.y;

			return r;
			}

		CVector operator * (double k) const
		{
			CVector r;

			r.x = x * k;
			r.y = y * k;

			return r;
			}

		double operator * (CVector const &v) const
		{
			return x * v.x + y * v.y;
			}

		double Mod(void) const
		{
			return sqrt(x * x + y * y);
			}

		double ModSq(void) const
		{
			return x * x + y * y;
			}
	};

// End of File

#endif
