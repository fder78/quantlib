/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2009 Klaus Spanderen

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

#include <ql/math/functional.hpp>
#include <ql/processes/hullwhiteprocess.hpp>
#include <ql/math/distributions/normaldistribution.hpp>
#include <ql/experimental/finitedifferences/fdmhullwhitemesher.hpp>

namespace QuantLib {

    FdmHullWhiteMesher::FdmHullWhiteMesher(
        Size size,
        const boost::shared_ptr<HullWhiteProcess>& process,
        Time maturity, Size tAvgSteps, Real eps)
        : Fdm1dMesher(size) {
            
        std::fill(locations_.begin(), locations_.end(), 0.0);    

		Real std = 0, mean = 0;		
		Real center = process->x0();

		for (Size l=1; l<=tAvgSteps; ++l) {
			Real z = InverseCumulativeNormal()(eps);
            const Real t = (maturity*l)/tAvgSteps;
			mean = process->expectation(0, process->x0(), t);
			std = process->stdDeviation(0, process->x0(), t);
            Real qMin = std::min(process->x0(), mean + std*z);
            Real qMax = std::max(process->x0(), mean - std*z);			

            const Real dp = (1-2*eps)/(size-1);
            Real p = eps;
			
			Real M = 0.5*(qMin+qMax);
			center = 0.5*(process->x0() + M);
			Real A = qMax - M;
			Real B = qMax - center;
			qMin = center + B/A*(qMin-M);
			qMax = center + B/A*(qMax-M);

			locations_[0] += qMin;   
            for (Size i=1; i < size-1; ++i) {
                p += dp;
				z = InverseCumulativeNormal()(p);
                locations_[i] += center + B/A*(mean+std*z - M);
            }
            locations_.back() += qMax;
			
        }

        std::transform(locations_.begin(), locations_.end(), locations_.begin(),
                       std::bind2nd(std::divides<Real>(), (Real)(tAvgSteps)));
        for (Size i=0; i < size-1; ++i) {
            dminus_[i+1] = dplus_[i] = locations_[i+1] - locations_[i];
        }
        dplus_.back() = dminus_.front() = Null<Real>();
    }

	FdmHullWhiteMesher::FdmHullWhiteMesher(
		Size size,
		const boost::shared_ptr<Generalized_HullWhite>& hw,
		Time maturity, Size tAvgSteps, Real eps)
		: Fdm1dMesher(size) {

			std::fill(locations_.begin(), locations_.end(), 0.0);    

			Real std = 0, mean = 0;		
			Real center = hw->x0();

			for (Size l=1; l<=tAvgSteps; ++l) {
				Real z = InverseCumulativeNormal()(eps);
				const Real t = (maturity*l)/tAvgSteps;
				mean = hw->expectation(0, t, hw->x0());
				std = hw->stdDeviation(0, t, hw->x0());
				Real qMin = std::min(hw->x0(), mean + std*z);
				Real qMax = std::max(hw->x0(), mean - std*z);			

				const Real dp = (1-2*eps)/(size-1);
				Real p = eps;

				Real M = 0.5*(qMin+qMax);
				center = 0.5*(hw->x0() + M);
				Real A = qMax - M;
				Real B = qMax - center;
				qMin = center + B/A*(qMin-M);
				qMax = center + B/A*(qMax-M);

				locations_[0] += qMin;   
				for (Size i=1; i < size-1; ++i) {
					p += dp;
					z = InverseCumulativeNormal()(p);
					locations_[i] += center + B/A*(mean+std*z - M);
				}
				locations_.back() += qMax;

			}

			std::transform(locations_.begin(), locations_.end(), locations_.begin(),
				std::bind2nd(std::divides<Real>(), (Real)(tAvgSteps)));
			for (Size i=0; i < size-1; ++i) {
				dminus_[i+1] = dplus_[i] = locations_[i+1] - locations_[i];
			}
			dplus_.back() = dminus_.front() = Null<Real>();
	}



}
