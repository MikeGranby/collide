
#include "Intern.hpp"

//////////////////////////////////////////////////////////////////////////
//
// Fun with Graphics 
//
// Copyright (c) 2017 Granby Consulting LLC
//
// All Rights Reserved
//

#ifndef INCLUDE_GDIPLUS_HPP

#define INCLUDE_GDIPLUS_HPP

//////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//

struct IDirectDrawSurface7;

//////////////////////////////////////////////////////////////////////////
//
// Gdi+ Flat APIs
//

namespace Gdiplus
{
	namespace DllExports
	{
		#include "GdiplusMem.h"
		};

	#include "GdiplusBase.h"
	#include "GdiplusEnums.h"
	#include "GdiplusTypes.h"
	#include "GdiplusInit.h"
	#include "GdiplusPixelFormats.h"
	#include "GdiplusColor.h"
	#include "GdiplusMetaHeader.h"
	#include "GdiplusImaging.h"
	#include "GdiplusColorMatrix.h"
	#include "GdiplusGpStubs.h"
	#include "GdiplusHeaders.h"

	namespace DllExports
	{
		#include "GdiplusFlat.h"
		};
	};

//////////////////////////////////////////////////////////////////////////
//
// Namespace Selection
//

using namespace Gdiplus;

using namespace Gdiplus::DllExports;

//////////////////////////////////////////////////////////////////////////
//
// Library Files
//

#pragma	comment(lib, "gdiplus.lib")

// End of File

#endif
