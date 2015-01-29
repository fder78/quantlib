#pragma once

#include <ds_interestrate_derivatives/instruments/notes/range_accrual_note.hpp>

#include <ql/processes/blackscholesprocess.hpp>
#include <ds_interestrate_derivatives/model/generalized_hullwhite.hpp>

#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/methods/finitedifferences/solvers/fdmhestonhullwhitesolver.hpp>

namespace QuantLib {

	class FdmR2HWSolver;
	class Fdm_R2_Dual_RA_Engine : public RangeAccrualNote::engine {

	public:
		// Constructor
		Fdm_R2_Dual_RA_Engine(
			const boost::shared_ptr<Generalized_HullWhite>& hw0,
			const boost::shared_ptr<Generalized_HullWhite>& hw1,
			const Handle<YieldTermStructure>& discTS,
			Real corrXR,
			Real pastAccrual,
			Size tGrid = 40, 
			Size r0Grid = 40, 
			Size r1Grid = 40,
			Size dampingSteps = 0,
			bool controlVariate = true,
			const FdmSchemeDesc& schemeDesc = FdmSchemeDesc::ModifiedCraigSneyd());

		void calculate() const;

		virtual void FetchResult( Real sLevel ) const;

	private:
		mutable boost::shared_ptr<FdmR2HWSolver> solver_;
		const boost::shared_ptr<Generalized_HullWhite> hw0_;
		const boost::shared_ptr<Generalized_HullWhite> hw1_;
		const Handle<YieldTermStructure> discTS_;
		const Real corrXR_;

		const Size tGrid_, xGrid_, rGrid_;
		const Size dampingSteps_;
		const FdmSchemeDesc schemeDesc_;
		const bool controlVariate_;
		Real pastAccrual_;

	};
}
