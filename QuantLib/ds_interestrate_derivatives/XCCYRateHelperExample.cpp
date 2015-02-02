/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*!
 Copyright (C) 2013 A. Miemiec, d-fine GmbH

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*  This example shows how to use the XCCY-RateHelper class to bootstrap 
    a yield curve from constant notional cross currency swaps.
*/

#include <ql/quantlib.hpp>


#include <iostream>
#include <iomanip>

#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

using namespace std;
using namespace QuantLib;



#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>


int main(int, char* []) {

    try {


		 /*********************
         ***  MARKET DATA  ***
         *********************/

		Calendar USDCalendar = UnitedStates();
        Calendar EURCalendar = TARGET();

		//valuation date
		Date today(2,August, 2012);

		// nothing to do with Date::todaysDate
        Settings::instance().evaluationDate() = today;


		std::vector<Date> dates;  
		std::vector<Real> values; 

        //USD-OIS		

		dates.clear(); values.clear();

		dates.push_back(Date(2,August,2012));values.push_back(1.0);
		dates.push_back(Date(3,August,2012));values.push_back(0.99999611);
		dates.push_back(Date(13,August,2012));values.push_back(0.99995694);
		dates.push_back(Date(20,August,2012));values.push_back(0.99992953);
		dates.push_back(Date(27,August,2012));values.push_back(0.99990212);
		dates.push_back(Date(6,September,2012));values.push_back(0.99986296);
		dates.push_back(Date(9,October,2012));values.push_back(0.99973199);
		dates.push_back(Date(6,November,2012));values.push_back(0.99962417);
		dates.push_back(Date(6,December,2012));values.push_back(0.99951015);
		dates.push_back(Date(7,January,2013));values.push_back(0.99939439);
		dates.push_back(Date(6,February,2013));values.push_back(0.99927954);
		dates.push_back(Date(6,March,2013));values.push_back(0.99917824);
		dates.push_back(Date(8,April,2013));values.push_back(0.99904609);
		dates.push_back(Date(6,May,2013));values.push_back(0.99893141);
		dates.push_back(Date(6,June,2013));values.push_back(0.99880355);
		dates.push_back(Date(8,July,2013));values.push_back(0.99867011);
		dates.push_back(Date(6,August,2013));values.push_back(0.99854673);
		dates.push_back(Date(6,November,2013));values.push_back(0.99812121);
		dates.push_back(Date(6,February,2014));values.push_back(0.99764011);
		dates.push_back(Date(6,May,2014));values.push_back(0.99706643);
		dates.push_back(Date(6,August,2014));values.push_back(0.99632349);
		dates.push_back(Date(6,August,2015));values.push_back(0.99174792);
		dates.push_back(Date(8,August,2016));values.push_back(0.98288518);
		dates.push_back(Date(7,August,2017));values.push_back(0.9684586);
		dates.push_back(Date(6,August,2018));values.push_back(0.94951998);
		dates.push_back(Date(6,August,2019));values.push_back(0.92774615);
		dates.push_back(Date(6,August,2020));values.push_back(0.90404409);
		dates.push_back(Date(6,August,2021));values.push_back(0.87968371);
		dates.push_back(Date(8,August,2022));values.push_back(0.85471464);
		dates.push_back(Date(7,August,2023));values.push_back(0.82874032);
		dates.push_back(Date(6,August,2024));values.push_back(0.80348723);
		dates.push_back(Date(6,August,2025));values.push_back(0.77939641);
		dates.push_back(Date(6,August,2026));values.push_back(0.7560279);
		dates.push_back(Date(6,August,2027));values.push_back(0.73336004);
		dates.push_back(Date(7,August,2028));values.push_back(0.71210711);
		dates.push_back(Date(6,August,2029));values.push_back(0.69163634);
		dates.push_back(Date(6,August,2030));values.push_back(0.67170021);
		dates.push_back(Date(6,August,2031));values.push_back(0.65233874);
		dates.push_back(Date(6,August,2032));values.push_back(0.63348458);
		dates.push_back(Date(8,August,2033));values.push_back(0.6155204);
		dates.push_back(Date(7,August,2034));values.push_back(0.59820631);
		dates.push_back(Date(6,August,2035));values.push_back(0.58137924);
		dates.push_back(Date(6,August,2036));values.push_back(0.56493694);
		dates.push_back(Date(6,August,2037));values.push_back(0.54900268);
		dates.push_back(Date(6,August,2038));values.push_back(0.5337537);
		dates.push_back(Date(8,August,2039));values.push_back(0.51884819);
		dates.push_back(Date(6,August,2040));values.push_back(0.50447572);
		dates.push_back(Date(6,August,2041));values.push_back(0.49046351);
		dates.push_back(Date(6,August,2042));values.push_back(0.47684051);
		dates.push_back(Date(6,August,2043));values.push_back(0.46421731);
		dates.push_back(Date(8,August,2044));values.push_back(0.45182862);
		dates.push_back(Date(7,August,2045));values.push_back(0.43989988);
		dates.push_back(Date(6,August,2046));values.push_back(0.42828607);
		dates.push_back(Date(6,August,2047));values.push_back(0.41694822);
		dates.push_back(Date(6,August,2048));values.push_back(0.4066303);
		dates.push_back(Date(6,August,2049));values.push_back(0.39659486);
		dates.push_back(Date(8,August,2050));values.push_back(0.38675413);
		dates.push_back(Date(7,August,2051));values.push_back(0.37723505);
		dates.push_back(Date(6,August,2052));values.push_back(0.36792507);
		dates.push_back(Date(6,August,2053));values.push_back(0.35948598);
		dates.push_back(Date(6,August,2054));values.push_back(0.35124046);
		dates.push_back(Date(6,August,2055));values.push_back(0.34318406);
		dates.push_back(Date(7,August,2056));values.push_back(0.33526982);
		dates.push_back(Date(6,August,2057));values.push_back(0.32760057);
		dates.push_back(Date(6,August,2058));values.push_back(0.3200864);
		dates.push_back(Date(6,August,2059));values.push_back(0.31274459);
		dates.push_back(Date(6,August,2060));values.push_back(0.30555174);
		dates.push_back(Date(8,August,2061));values.push_back(0.29850535);
		dates.push_back(Date(7,August,2062));values.push_back(0.29167708);

		Handle<YieldTermStructure> YC_USD_OIS = Handle<YieldTermStructure>(new DiscountCurve(dates,values,Actual365Fixed()));


		//USD-3M-FWD

		dates.clear(); values.clear();

		dates.push_back(Date(2,August,2012));values.push_back(1);
		dates.push_back(Date(6,August,2012));values.push_back(0.99995093);
		dates.push_back(Date(6,September,2012));values.push_back(0.99957076);
		dates.push_back(Date(9,October,2012));values.push_back(0.99916621);
		dates.push_back(Date(6,November,2012));values.push_back(0.99882309);
		dates.push_back(Date(6,December,2012));values.push_back(0.99852454);
		dates.push_back(Date(7,January,2013));values.push_back(0.99813688);
		dates.push_back(Date(6,February,2013));values.push_back(0.99777255);
		dates.push_back(Date(6,March,2013));values.push_back(0.99750459);
		dates.push_back(Date(8,April,2013));values.push_back(0.99710602);
		dates.push_back(Date(7,May,2013));values.push_back(0.99673843);
		dates.push_back(Date(6,June,2013));values.push_back(0.99643508);
		dates.push_back(Date(8,July,2013));values.push_back(0.99603597);
		dates.push_back(Date(7,August,2013));values.push_back(0.99566458);
		dates.push_back(Date(6,November,2013));values.push_back(0.99452631);
		dates.push_back(Date(6,February,2014));values.push_back(0.99328644);
		dates.push_back(Date(6,May,2014));values.push_back(0.99192455);
		dates.push_back(Date(6,August,2014));values.push_back(0.99051871);
		dates.push_back(Date(6,November,2014));values.push_back(0.98867745);
		dates.push_back(Date(6,February,2015));values.push_back(0.98683962);
		dates.push_back(Date(6,May,2015));values.push_back(0.98506497);
		dates.push_back(Date(6,August,2015));values.push_back(0.98323385);
		dates.push_back(Date(6,November,2015));values.push_back(0.9803148);
		dates.push_back(Date(8,February,2016));values.push_back(0.97734125);
		dates.push_back(Date(9,May,2016));values.push_back(0.97447118);
		dates.push_back(Date(8,August,2016));values.push_back(0.97160955);
		dates.push_back(Date(8,November,2016));values.push_back(0.9673523);
		dates.push_back(Date(7,February,2017));values.push_back(0.96315968);
		dates.push_back(Date(8,May,2017));values.push_back(0.959031);
		dates.push_back(Date(8,August,2017));values.push_back(0.95482886);
		dates.push_back(Date(7,November,2017));values.push_back(0.94953539);
		dates.push_back(Date(6,February,2018));values.push_back(0.94427126);
		dates.push_back(Date(8,May,2018));values.push_back(0.93903631);
		dates.push_back(Date(8,August,2018));values.push_back(0.93377334);
		dates.push_back(Date(6,November,2018));values.push_back(0.92786652);
		dates.push_back(Date(6,February,2019));values.push_back(0.92186706);
		dates.push_back(Date(7,May,2019));values.push_back(0.91603557);
		dates.push_back(Date(7,August,2019));values.push_back(0.9101126);
		dates.push_back(Date(6,November,2019));values.push_back(0.90379868);
		dates.push_back(Date(6,February,2020));values.push_back(0.89745989);
		dates.push_back(Date(6,May,2020));values.push_back(0.89130193);
		dates.push_back(Date(6,August,2020));values.push_back(0.88505079);
		dates.push_back(Date(6,November,2020));values.push_back(0.87848151);
		dates.push_back(Date(8,February,2021));values.push_back(0.87181978);
		dates.push_back(Date(10,May,2021));values.push_back(0.86541878);
		dates.push_back(Date(6,August,2021));values.push_back(0.85927351);
		dates.push_back(Date(8,November,2021));values.push_back(0.85243901);
		dates.push_back(Date(8,February,2022));values.push_back(0.84580256);
		dates.push_back(Date(9,May,2022));values.push_back(0.83936038);
		dates.push_back(Date(8,August,2022));values.push_back(0.83289651);
		dates.push_back(Date(8,November,2022));values.push_back(0.8260887);
		dates.push_back(Date(7,February,2023));values.push_back(0.81940963);
		dates.push_back(Date(8,May,2023));values.push_back(0.81285707);
		dates.push_back(Date(8,August,2023));values.push_back(0.80621305);
		dates.push_back(Date(7,November,2023));values.push_back(0.79969467);
		dates.push_back(Date(6,February,2024));values.push_back(0.793229);
		dates.push_back(Date(7,May,2024));values.push_back(0.78681561);
		dates.push_back(Date(7,August,2024));values.push_back(0.78038444);
		dates.push_back(Date(6,November,2024));values.push_back(0.77408378);
		dates.push_back(Date(6,February,2025));values.push_back(0.76776559);
		dates.push_back(Date(6,May,2025));values.push_back(0.76170251);
		dates.push_back(Date(6,August,2025));values.push_back(0.75548537);
		dates.push_back(Date(6,November,2025));values.push_back(0.74931899);
		dates.push_back(Date(6,February,2026));values.push_back(0.74320293);
		dates.push_back(Date(6,May,2026));values.push_back(0.73733382);
		dates.push_back(Date(6,August,2026));values.push_back(0.73131559);
		dates.push_back(Date(6,November,2026));values.push_back(0.72534648);
		dates.push_back(Date(8,February,2027));values.push_back(0.71929792);
		dates.push_back(Date(10,May,2027));values.push_back(0.71349046);
		dates.push_back(Date(6,August,2027));values.push_back(0.70791905);
		dates.push_back(Date(8,November,2027));values.push_back(0.70231264);
		dates.push_back(Date(8,February,2028));values.push_back(0.69686851);
		dates.push_back(Date(8,May,2028));values.push_back(0.69158357);
		dates.push_back(Date(8,August,2028));values.push_back(0.68622261);
		dates.push_back(Date(7,November,2028));values.push_back(0.68096081);
		dates.push_back(Date(6,February,2029));values.push_back(0.67573935);
		dates.push_back(Date(8,May,2029));values.push_back(0.67055792);
		dates.push_back(Date(8,August,2029));values.push_back(0.66535995);
		dates.push_back(Date(6,November,2029));values.push_back(0.66031396);
		dates.push_back(Date(6,February,2030));values.push_back(0.6551954);
		dates.push_back(Date(7,May,2030));values.push_back(0.6502265);
		dates.push_back(Date(7,August,2030));values.push_back(0.64518613);
		dates.push_back(Date(6,November,2030));values.push_back(0.64023898);
		dates.push_back(Date(6,February,2031));values.push_back(0.63527603);
		dates.push_back(Date(6,May,2031));values.push_back(0.63051153);
		dates.push_back(Date(6,August,2031));values.push_back(0.62562398);
		dates.push_back(Date(6,November,2031));values.push_back(0.62077432);
		dates.push_back(Date(6,February,2032));values.push_back(0.61596225);
		dates.push_back(Date(6,May,2032));values.push_back(0.61129089);
		dates.push_back(Date(6,August,2032));values.push_back(0.60655234);
		dates.push_back(Date(8,November,2032));values.push_back(0.60190123);
		dates.push_back(Date(8,February,2033));values.push_back(0.59738362);
		dates.push_back(Date(9,May,2033));values.push_back(0.59299703);
		dates.push_back(Date(8,August,2033));values.push_back(0.58859445);
		dates.push_back(Date(8,November,2033));values.push_back(0.58417671);
		dates.push_back(Date(7,February,2034));values.push_back(0.57983961);
		dates.push_back(Date(8,May,2034));values.push_back(0.57558185);
		dates.push_back(Date(8,August,2034));values.push_back(0.57126178);
		dates.push_back(Date(7,November,2034));values.push_back(0.56702057);
		dates.push_back(Date(6,February,2035));values.push_back(0.56281084);
		dates.push_back(Date(8,May,2035));values.push_back(0.55863237);
		dates.push_back(Date(8,August,2035));values.push_back(0.55443952);
		dates.push_back(Date(6,November,2035));values.push_back(0.55036826);
		dates.push_back(Date(6,February,2036));values.push_back(0.54623744);
		dates.push_back(Date(6,May,2036));values.push_back(0.54222641);
		dates.push_back(Date(6,August,2036));values.push_back(0.5381567);
		dates.push_back(Date(6,November,2036));values.push_back(0.53411752);
		dates.push_back(Date(6,February,2037));values.push_back(0.53010867);
		dates.push_back(Date(6,May,2037));values.push_back(0.52625917);
		dates.push_back(Date(6,August,2037));values.push_back(0.5223093);
		dates.push_back(Date(6,November,2037));values.push_back(0.51838776);
		dates.push_back(Date(8,February,2038));values.push_back(0.51441138);
		dates.push_back(Date(10,May,2038));values.push_back(0.51059096);
		dates.push_back(Date(6,August,2038));values.push_back(0.50692348);
		dates.push_back(Date(8,November,2038));values.push_back(0.50303503);
		dates.push_back(Date(8,February,2039));values.push_back(0.49925821);
		dates.push_back(Date(9,May,2039));values.push_back(0.49559093);
		dates.push_back(Date(8,August,2039));values.push_back(0.49191028);
		dates.push_back(Date(8,November,2039));values.push_back(0.48821698);
		dates.push_back(Date(7,February,2040));values.push_back(0.48459111);
		dates.push_back(Date(7,May,2040));values.push_back(0.48103156);
		dates.push_back(Date(7,August,2040));values.push_back(0.47741994);
		dates.push_back(Date(6,November,2040));values.push_back(0.47387425);
		dates.push_back(Date(6,February,2041));values.push_back(0.47031636);
		dates.push_back(Date(6,May,2041));values.push_back(0.46689992);
		dates.push_back(Date(6,August,2041));values.push_back(0.46339439);
		dates.push_back(Date(6,November,2041));values.push_back(0.45991519);
		dates.push_back(Date(6,February,2042));values.push_back(0.45646211);
		dates.push_back(Date(6,May,2042));values.push_back(0.45314631);
		dates.push_back(Date(6,August,2042));values.push_back(0.44974405);
		dates.push_back(Date(6,November,2042));values.push_back(0.44670095);
		dates.push_back(Date(6,February,2043));values.push_back(0.44367844);
		dates.push_back(Date(6,May,2043));values.push_back(0.44077396);
		dates.push_back(Date(6,August,2043));values.push_back(0.43779156);
		dates.push_back(Date(6,November,2043));values.push_back(0.43482933);
		dates.push_back(Date(8,February,2044));values.push_back(0.43182341);
		dates.push_back(Date(9,May,2044));values.push_back(0.42893322);
		dates.push_back(Date(8,August,2044));values.push_back(0.42606238);
		dates.push_back(Date(8,November,2044));values.push_back(0.42317952);
		dates.push_back(Date(7,February,2045));values.push_back(0.42034718);
		dates.push_back(Date(8,May,2045));values.push_back(0.41756462);
		dates.push_back(Date(8,August,2045));values.push_back(0.41473925);
		dates.push_back(Date(7,November,2045));values.push_back(0.41196341);
		dates.push_back(Date(6,February,2046));values.push_back(0.40920614);
		dates.push_back(Date(7,May,2046));values.push_back(0.40649733);
		dates.push_back(Date(7,August,2046));values.push_back(0.40374685);
		dates.push_back(Date(6,November,2046));values.push_back(0.40104458);
		dates.push_back(Date(6,February,2047));values.push_back(0.39833099);
		dates.push_back(Date(6,May,2047));values.push_back(0.39572337);
		dates.push_back(Date(6,August,2047));values.push_back(0.39304579);
		dates.push_back(Date(6,November,2047));values.push_back(0.39038633);
		dates.push_back(Date(6,February,2048));values.push_back(0.38774487);
		dates.push_back(Date(6,May,2048));values.push_back(0.38517812);
		dates.push_back(Date(6,August,2048));values.push_back(0.38257189);
		dates.push_back(Date(6,November,2048));values.push_back(0.3799833);
		dates.push_back(Date(8,February,2049));values.push_back(0.37735652);
		dates.push_back(Date(10,May,2049));values.push_back(0.37483088);
		dates.push_back(Date(6,August,2049));values.push_back(0.37240458);
		dates.push_back(Date(8,November,2049));values.push_back(0.3698302);
		dates.push_back(Date(8,February,2050));values.push_back(0.36732782);
		dates.push_back(Date(9,May,2050));values.push_back(0.36489622);
		dates.push_back(Date(8,August,2050));values.push_back(0.36245398);
		dates.push_back(Date(8,November,2050));values.push_back(0.36000151);
		dates.push_back(Date(7,February,2051));values.push_back(0.35759202);
		dates.push_back(Date(8,May,2051));values.push_back(0.35522488);
		dates.push_back(Date(8,August,2051));values.push_back(0.35282132);
		dates.push_back(Date(7,November,2051));values.push_back(0.3504599);
		dates.push_back(Date(6,February,2052));values.push_back(0.34811427);
		dates.push_back(Date(6,May,2052));values.push_back(0.34580987);
		dates.push_back(Date(6,August,2052));values.push_back(0.34347002);
		dates.push_back(Date(6,November,2052));values.push_back(0.34139925);
		dates.push_back(Date(6,February,2053));values.push_back(0.33934098);
		dates.push_back(Date(6,May,2053));values.push_back(0.33736163);
		dates.push_back(Date(6,August,2053));values.push_back(0.33532769);
		dates.push_back(Date(6,November,2053));values.push_back(0.33330602);
		dates.push_back(Date(6,February,2054));values.push_back(0.33129653);
		dates.push_back(Date(6,May,2054));values.push_back(0.32936411);
		dates.push_back(Date(6,August,2054));values.push_back(0.32737839);
		dates.push_back(Date(6,November,2054));values.push_back(0.32540464);
		dates.push_back(Date(8,February,2055));values.push_back(0.32340028);
		dates.push_back(Date(10,May,2055));values.push_back(0.32147164);
		dates.push_back(Date(6,August,2055));values.push_back(0.31961753);
		dates.push_back(Date(8,November,2055));values.push_back(0.31764881);
		dates.push_back(Date(8,February,2056));values.push_back(0.31573373);
		dates.push_back(Date(8,May,2056));values.push_back(0.31387144);
		dates.push_back(Date(8,August,2056));values.push_back(0.31197913);
		dates.push_back(Date(7,November,2056));values.push_back(0.31011861);
		dates.push_back(Date(6,February,2057));values.push_back(0.30826918);
		dates.push_back(Date(7,May,2057));values.push_back(0.30645093);
		dates.push_back(Date(7,August,2057));values.push_back(0.30460335);
		dates.push_back(Date(6,November,2057));values.push_back(0.30278682);
		dates.push_back(Date(6,February,2058));values.push_back(0.30096133);
		dates.push_back(Date(6,May,2058));values.push_back(0.29920585);
		dates.push_back(Date(6,August,2058));values.push_back(0.29740195);
		dates.push_back(Date(6,November,2058));values.push_back(0.29560893);
		dates.push_back(Date(6,February,2059));values.push_back(0.29382672);
		dates.push_back(Date(6,May,2059));values.push_back(0.29211285);
		dates.push_back(Date(6,August,2059));values.push_back(0.29035172);
		dates.push_back(Date(6,November,2059));values.push_back(0.2886012);
		dates.push_back(Date(6,February,2060));values.push_back(0.28686124);
		dates.push_back(Date(6,May,2060));values.push_back(0.28516926);
		dates.push_back(Date(6,August,2060));values.push_back(0.28344999);
		dates.push_back(Date(8,November,2060));values.push_back(0.28170405);
		dates.push_back(Date(8,February,2061));values.push_back(0.28000567);
		dates.push_back(Date(9,May,2061));values.push_back(0.27835412);
		dates.push_back(Date(8,August,2061));values.push_back(0.27669412);
		dates.push_back(Date(8,November,2061));values.push_back(0.27502595);
		dates.push_back(Date(7,February,2062));values.push_back(0.2733858);
		dates.push_back(Date(8,May,2062));values.push_back(0.2717733);
		dates.push_back(Date(8,August,2062));values.push_back(0.27013479);

		Handle<YieldTermStructure> YC_USD_3M = Handle<YieldTermStructure>(new DiscountCurve(dates,values,Actual365Fixed()));


		//EUR-3M-FWD

		dates.clear(); values.clear();

		dates.push_back(Date(2,August,2012));values.push_back(1);
		dates.push_back(Date(6,August,2012));values.push_back(0.99995835);
		dates.push_back(Date(6,September,2012));values.push_back(0.99963566);
		dates.push_back(Date(8,October,2012));values.push_back(0.99930266);
		dates.push_back(Date(6,November,2012));values.push_back(0.99900098);
		dates.push_back(Date(6,December,2012));values.push_back(0.99879744);
		dates.push_back(Date(7,January,2013));values.push_back(0.99852273);
		dates.push_back(Date(6,February,2013));values.push_back(0.99823566);
		dates.push_back(Date(6,March,2013));values.push_back(0.99807135);
		dates.push_back(Date(8,April,2013));values.push_back(0.9978165);
		dates.push_back(Date(6,May,2013));values.push_back(0.99753775);
		dates.push_back(Date(6,June,2013));values.push_back(0.99733985);
		dates.push_back(Date(8,July,2013));values.push_back(0.99708558);
		dates.push_back(Date(6,August,2013));values.push_back(0.99679542);
		dates.push_back(Date(6,November,2013));values.push_back(0.99600613);
		dates.push_back(Date(6,February,2014));values.push_back(0.9951133);
		dates.push_back(Date(6,May,2014));values.push_back(0.99415204);
		dates.push_back(Date(6,August,2014));values.push_back(0.99296576);
		dates.push_back(Date(6,August,2015));values.push_back(0.98661549);
		dates.push_back(Date(8,August,2016));values.push_back(0.97609743);
		dates.push_back(Date(8,August,2017));values.push_back(0.96106956);
		dates.push_back(Date(7,August,2018));values.push_back(0.94192738);
		dates.push_back(Date(6,August,2019));values.push_back(0.92072502);
		dates.push_back(Date(6,August,2020));values.push_back(0.89822478);
		dates.push_back(Date(6,August,2021));values.push_back(0.87530006);
		dates.push_back(Date(8,August,2022));values.push_back(0.85176186);
		dates.push_back(Date(8,August,2023));values.push_back(0.82798794);
		dates.push_back(Date(6,August,2024));values.push_back(0.8043592);
		dates.push_back(Date(6,August,2027));values.push_back(0.73944529);
		dates.push_back(Date(6,August,2032));values.push_back(0.65860867);
		dates.push_back(Date(6,August,2037));values.push_back(0.5909934);
		dates.push_back(Date(6,August,2042));values.push_back(0.53015423);
		dates.push_back(Date(6,August,2052));values.push_back(0.40585899);
		dates.push_back(Date(8,August,2062));values.push_back(0.30694591);


		Handle<YieldTermStructure> YC_EUR_3M = Handle<YieldTermStructure>(new DiscountCurve(dates,values,Actual365Fixed()));



        //setup Bootstrap Helpers

		Real fxRate = 1.2346;

        Period lengths[] = { Period(3,Months),Period(6,Months),Period(9,Months),Period(1,Years),
			                 Period(18,Months),Period(2,Years),Period(3,Years),Period(4,Years),
							 Period(5,Years),Period(7,Years),Period(10,Years),Period(15,Years),
							 Period(20,Years),Period(30,Years) };

		Real spreads[] = { 0.0043,  0.00435,0.0044, 0.0045, 0.0047, 0.004775,0.004725,
			               0.004625,0.00445,0.00395,0.003175,0.0024,0.002025,0.00165 };


		boost::shared_ptr<IborIndex> flatLegIborIndex(new USDLibor(Period(3,Months),YC_USD_3M));
		flatLegIborIndex->addFixing(Date(2,August,2012),0.0044185);
        boost::shared_ptr<IborIndex> sprdLegIborIndex(new Euribor3M(YC_EUR_3M));
        sprdLegIborIndex->addFixing(Date(2,August,2012),0.00375);

		std::vector<boost::shared_ptr<RateHelper>> instruments; instruments.empty();

		Handle<YieldTermStructure> flatLegDiscTermStructureHandle = YC_USD_OIS;
		Handle<YieldTermStructure> sprdLegDiscTermStructureHandle;

		for(Size i=0;i<14;i++){

			Handle<Quote> MarketSpread =  Handle<Quote>(new SimpleQuote(-spreads[i]));

            boost::shared_ptr<XCCySwapRateHelper> XCCYHelper(
                     new  XCCySwapRateHelper(MarketSpread,
			                lengths[i],
			                //flat leg
							USDCurrency(),
							UnitedStates(),
							ModifiedFollowing,
							Actual360(),
							flatLegIborIndex,
							flatLegDiscTermStructureHandle,
							//spread leg
							EURCurrency(),
							TARGET(),
							ModifiedFollowing,
							Actual360(),
							sprdLegIborIndex,
							sprdLegDiscTermStructureHandle,
							//general
							fxRate,
							Period(0,Days), 
							XCCySwapRateHelper::BootstrapSpreadDiscCurve));

            instruments.push_back(XCCYHelper);
        }



		//Bootstrap XCCY-Curve


        bool constrainAtZero = true;
        Real tolerance = 1.0e-10;
        Size max = 5000;

        
 		Natural settlementDays = 0;
        Calendar calendar   = TARGET();
        DayCounter dc = Actual365Fixed();


        Date referenceDate = calendar.advance (today , settlementDays , Days );

		boost :: shared_ptr < YieldTermStructure > yieldCurve = boost :: shared_ptr < YieldTermStructure >( new
                         PiecewiseYieldCurve < Discount , LogLinear >( referenceDate , instruments , dc ));


        DiscountFactor df = yieldCurve->discount(today,true);

		cout << "Difference of discount factors between Reference system and QuantLib:\n";
		cout << "06.11.2012 : " << 10000*(1.000948806-yieldCurve->discount(Date(6,November,2012),true)) << "\n";
		cout << "06.02.2013 : " << 10000*(1.002014821-yieldCurve->discount(Date(6,February,2013),true)) << "\n";
		cout << "07.05.2013 : " << 10000*(1.003118318-yieldCurve->discount(Date(7,May,2013),true)) << "\n";
		cout << "06.08.2013 : " << 10000*(1.004279715-yieldCurve->discount(Date(6,August,2013),true)) << "\n";
		cout << "06.02.2014 : " << 10000*(1.006693419-yieldCurve->discount(Date(6,February,2014),true)) << "\n";
		cout << "06.08.2014 : " << 10000*(1.008515717-yieldCurve->discount(Date(6,August,2014),true)) << "\n";
		cout << "06.08.2015 : " << 10000*(1.009533464-yieldCurve->discount(Date(6,August,2015),true)) << "\n";
		cout << "08.08.2016 : " << 10000*(1.006062360-yieldCurve->discount(Date(8,August,2016),true)) << "\n";
		cout << "07.08.2017 : " << 10000*(0.996869768-yieldCurve->discount(Date(7,August,2017),true)) << "\n";
		cout << "06.08.2019 : " << 10000*(0.964780619-yieldCurve->discount(Date(6,August,2019),true)) << "\n";
		cout << "08.08.2022 : " << 10000*(0.901714283-yieldCurve->discount(Date(8,August,2022),true)) << "\n";
		cout << "06.08.2027 : " << 10000*(0.792102707-yieldCurve->discount(Date(6,August,2027),true)) << "\n";
		cout << "06.08.2032 : " << 10000*(0.712995249-yieldCurve->discount(Date(6,August,2032),true)) << "\n";
		cout << "06.08.2042 : " << 10000*(0.584926095-yieldCurve->discount(Date(6,August,2042),true)) << "\n";

        return 0;

    } catch (std::exception& e) {
        cerr << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "unknown error" << endl;
        return 1;
    }

}

