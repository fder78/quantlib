#include "StdAfx.h"

#include "RASpreadParam.h"
#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"
#include "yield_builder.hpp"
#include "BloombergCaller.h"
#include "ProductIndex.h"
#include "StringUtil.h"
#include "XMLValue.h"

#include "pricing_functions/hull_white_calibration.hpp"

void RASpreadParam::SetDataImpl( TiXmlElement* record )
{
	__super::SetDataImpl( record );
}

