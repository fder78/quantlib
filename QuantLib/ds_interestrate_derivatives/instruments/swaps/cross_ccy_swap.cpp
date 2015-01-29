
#include <ds_interestrate_derivatives/instruments/swaps/cross_ccy_swap.hpp>

#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/cashflows/iborcoupon.hpp>
#include <ql/cashflows/cashflowvectors.hpp>
#include <ql/cashflows/cashflows.hpp>
#include <ql/cashflows/couponpricer.hpp>
#include <ql/indexes/iborindex.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>

namespace QuantLib {

    CrossCCYSwap::VanillaSwap(
                     Type type,
                     Real nominal,
                     const Schedule& fixedSchedule,
                     Rate fixedRate,
                     const DayCounter& fixedDayCount,
                     const Schedule& floatSchedule,
                     const boost::shared_ptr<IborIndex>& iborIndex,
                     Spread spread,
                     const DayCounter& floatingDayCount,
                     boost::optional<BusinessDayConvention> paymentConvention)
    : Swap(2), type_(type), nominal_(nominal),
      fixedSchedule_(fixedSchedule), fixedRate_(fixedRate),
      fixedDayCount_(fixedDayCount),
      floatingSchedule_(floatSchedule), iborIndex_(iborIndex), spread_(spread),
      floatingDayCount_(floatingDayCount) {

        if (paymentConvention)
            paymentConvention_ = *paymentConvention;
        else
            paymentConvention_ = floatingSchedule_.businessDayConvention();

        legs_[0] = FixedRateLeg(fixedSchedule_)
            .withNotionals(nominal_)
            .withCouponRates(fixedRate_, fixedDayCount_)
            .withPaymentAdjustment(paymentConvention_);

        legs_[1] = IborLeg(floatingSchedule_, iborIndex_)
            .withNotionals(nominal_)
            .withPaymentDayCounter(floatingDayCount_)
            .withPaymentAdjustment(paymentConvention_)
            .withSpreads(spread_);
        for (Leg::const_iterator i = legs_[1].begin(); i < legs_[1].end(); ++i)
            registerWith(*i);

        switch (type_) {
          case Payer:
            payer_[0] = -1.0;
            payer_[1] = +1.0;
            break;
          case Receiver:
            payer_[0] = +1.0;
            payer_[1] = -1.0;
            break;
          default:
            QL_FAIL("Unknown vanilla-swap type");
        }
    }

    void VanillaSwap::setupArguments(PricingEngine::arguments* args) const {

        Swap::setupArguments(args);

        VanillaSwap::arguments* arguments =
            dynamic_cast<VanillaSwap::arguments*>(args);

        if (!arguments)  // it's a swap engine...
            return;

        arguments->type = type_;
        arguments->nominal = nominal_;

        const Leg& fixedCoupons = fixedLeg();

        arguments->fixedResetDates = arguments->fixedPayDates =
            std::vector<Date>(fixedCoupons.size());
        arguments->fixedCoupons = std::vector<Real>(fixedCoupons.size());

        for (Size i=0; i<fixedCoupons.size(); ++i) {
            boost::shared_ptr<FixedRateCoupon> coupon =
                boost::dynamic_pointer_cast<FixedRateCoupon>(fixedCoupons[i]);

            arguments->fixedPayDates[i] = coupon->date();
            arguments->fixedResetDates[i] = coupon->accrualStartDate();
            arguments->fixedCoupons[i] = coupon->amount();
        }

        const Leg& floatingCoupons = floatingLeg();

        arguments->floatingResetDates = arguments->floatingPayDates =
            arguments->floatingFixingDates =
            std::vector<Date>(floatingCoupons.size());
        arguments->floatingAccrualTimes =
            std::vector<Time>(floatingCoupons.size());
        arguments->floatingSpreads =
            std::vector<Spread>(floatingCoupons.size());
        arguments->floatingCoupons = std::vector<Real>(floatingCoupons.size());
        for (Size i=0; i<floatingCoupons.size(); ++i) {
            boost::shared_ptr<IborCoupon> coupon =
                boost::dynamic_pointer_cast<IborCoupon>(floatingCoupons[i]);

            arguments->floatingResetDates[i] = coupon->accrualStartDate();
            arguments->floatingPayDates[i] = coupon->date();

            arguments->floatingFixingDates[i] = coupon->fixingDate();
            arguments->floatingAccrualTimes[i] = coupon->accrualPeriod();
            arguments->floatingSpreads[i] = coupon->spread();
            try {
                arguments->floatingCoupons[i] = coupon->amount();
            } catch (Error&) {
                arguments->floatingCoupons[i] = Null<Real>();
            }
        }
    }

    Rate VanillaSwap::fairRate() const {
        calculate();
        QL_REQUIRE(fairRate_ != Null<Rate>(), "result not available");
        return fairRate_;
    }

    Spread VanillaSwap::fairSpread() const {
        calculate();
        QL_REQUIRE(fairSpread_ != Null<Spread>(), "result not available");
        return fairSpread_;
    }

    Real VanillaSwap::fixedLegBPS() const {
        calculate();
        QL_REQUIRE(legBPS_[0] != Null<Real>(), "result not available");
        return legBPS_[0];
    }

    Real VanillaSwap::floatingLegBPS() const {
        calculate();
        QL_REQUIRE(legBPS_[1] != Null<Real>(), "result not available");
        return legBPS_[1];
    }

    Real VanillaSwap::fixedLegNPV() const {
        calculate();
        QL_REQUIRE(legNPV_[0] != Null<Real>(), "result not available");
        return legNPV_[0];
    }

    Real VanillaSwap::floatingLegNPV() const {
        calculate();
        QL_REQUIRE(legNPV_[1] != Null<Real>(), "result not available");
        return legNPV_[1];
    }

    void VanillaSwap::setupExpired() const {
        Swap::setupExpired();
        legBPS_[0] = legBPS_[1] = 0.0;
        fairRate_ = Null<Rate>();
        fairSpread_ = Null<Spread>();
    }

    void VanillaSwap::fetchResults(const PricingEngine::results* r) const {
        static const Spread basisPoint = 1.0e-4;

        Swap::fetchResults(r);

        const VanillaSwap::results* results =
            dynamic_cast<const VanillaSwap::results*>(r);
        if (results) { // might be a swap engine, so no error is thrown
            fairRate_ = results->fairRate;
            fairSpread_ = results->fairSpread;
        } else {
            fairRate_ = Null<Rate>();
            fairSpread_ = Null<Spread>();
        }

        if (fairRate_ == Null<Rate>()) {
            // calculate it from other results
            if (legBPS_[0] != Null<Real>())
                fairRate_ = fixedRate_ - NPV_/(legBPS_[0]/basisPoint);
        }
        if (fairSpread_ == Null<Spread>()) {
            // ditto
            if (legBPS_[1] != Null<Real>())
                fairSpread_ = spread_ - NPV_/(legBPS_[1]/basisPoint);
        }
    }

    void CrossCCYSwap::arguments::validate() const {
        VanillaSwap::arguments::validate();
    }

}
