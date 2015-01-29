#ifndef fx_termstructure_hpp
#define fx_termstructure_hpp

#include <ql/handle.hpp>
#include <ql/quote.hpp>
#include <ql/termstructure.hpp>
#include <ql/termstructures/interpolatedcurve.hpp>
#include <ql/math/interpolations/loginterpolation.hpp>

namespace QuantLib {

    //! TermStructure based on interpolation of discount factors
    /*! /ingroup termstructures */

	class FxTermStructure : public TermStructure
	{
	public:

        FxTermStructure(
            const DayCounter& dc = DayCounter(),
            const std::vector<Handle<Quote> >& jumps = std::vector<Handle<Quote> >(),
			const std::vector<Date>& jumpDates = std::vector<Date>())
			: TermStructure(dc) {}
        FxTermStructure(
            const Date& referenceDate,
            const Calendar& calendar = Calendar(),
            const DayCounter& dc = DayCounter(),
            const std::vector<Handle<Quote> >& jumps = std::vector<Handle<Quote> >(),
            const std::vector<Date>& jumpDates = std::vector<Date>())
			: TermStructure(referenceDate, calendar, dc) {}
        FxTermStructure(
            Natural settlementDays,
            const Calendar& calendar,
            const DayCounter& dc = DayCounter(),
            const std::vector<Handle<Quote> >& jumps = std::vector<Handle<Quote> >(),
            const std::vector<Date>& jumpDates = std::vector<Date>())
			: TermStructure(settlementDays, calendar, dc) {}

		virtual Real getRate(Time t) const = 0;
		virtual Real getRate(Date date) const = 0;
	};

    template <class Interpolator>
    class InterpolatedFxCurve
        : public FxTermStructure,
          protected InterpolatedCurve<Interpolator> {
      public:
        InterpolatedFxCurve(
            const std::vector<Date>& dates,
            const std::vector<Real>& rates,
            const DayCounter& dayCounter,
            const Calendar& calendar,
            const Interpolator& interpolator = Interpolator());
        InterpolatedFxCurve(
            const std::vector<Date>& dates,
            const std::vector<Real>& rates,
            const DayCounter& dayCounter,
            const Interpolator& interpolator = Interpolator());
        //! /name TermStructure interface
        //@{
		Date maxDate() const{
			return dates_.back();
		}

		Real getRate(Time t) const;
		Real getRate(Date date) const;
        //@}

		
		const std::vector<Time>& times() const{
			return times_;
		}
		const std::vector<Date>& dates() const{
			return dates_;
		}
		const std::vector<Real>& data() const{
			return data_;
		}
		std::vector<std::pair<Date, Real> > nodes() const{
			std::vector<std::pair<Date, Real> > results(dates_.size());
			for (Size i=0; i<dates_.size(); ++i)
				results[i] = std::make_pair(dates_[i], data_[i]);
			return results;
		}

	protected:
        InterpolatedFxCurve(
            const DayCounter&,
            const std::vector<Handle<Quote> >& jumps = std::vector<Handle<Quote> >(),
            const std::vector<Date>& jumpDates = std::vector<Date>(),
            const Interpolator& interpolator = Interpolator());
        InterpolatedFxCurve(
            const Date& referenceDate,
            const DayCounter&,
            const std::vector<Handle<Quote> >& jumps = std::vector<Handle<Quote> >(),
            const std::vector<Date>& jumpDates = std::vector<Date>(),
            const Interpolator& interpolator = Interpolator());
        InterpolatedFxCurve(
            Natural settlementDays,
            const Calendar&,
            const DayCounter&,
            const std::vector<Handle<Quote> >& jumps = std::vector<Handle<Quote> >(),
            const std::vector<Date>& jumpDates = std::vector<Date>(),
            const Interpolator& interpolator = Interpolator());

        mutable std::vector<Date> dates_;
      private:
        void initialize();
    };

    //! Term structure based on log-linear interpolation of discount factors
    /*! Log-linear interpolation guarantees piecewise-constant forward
        rates.

        /ingroup yieldtermstructures
    */
    typedef InterpolatedFxCurve<LogLinear> FxLogTermStructure;


    // inline definitions

	template <class T>
    inline Real
    InterpolatedFxCurve<T>::getRate(Time t) const {
        return this->interpolation_(t, true);
    }

	template <class T>
    inline Real
    InterpolatedFxCurve<T>::getRate(Date date) const {
        return getRate(this->timeFromReference(date));
    }

	template <class T>
    InterpolatedFxCurve<T>::InterpolatedFxCurve(
                                    const DayCounter& dayCounter,
                                    const std::vector<Handle<Quote> >& jumps,
                                    const std::vector<Date>& jumpDates,
                                    const T& interpolator)
    : FxTermStructure(dayCounter, jumps, jumpDates),
      InterpolatedCurve<T>(interpolator) {}

    template <class T>
    InterpolatedFxCurve<T>::InterpolatedFxCurve(
                                    const Date& referenceDate,
                                    const DayCounter& dayCounter,
                                    const std::vector<Handle<Quote> >& jumps,
                                    const std::vector<Date>& jumpDates,
                                    const T& interpolator)
    : FxTermStructure(referenceDate, Calendar(), dayCounter, jumps, jumpDates),
      InterpolatedCurve<T>(interpolator) {}

    template <class T>
    InterpolatedFxCurve<T>::InterpolatedFxCurve(
                                    Natural settlementDays,
                                    const Calendar& calendar,
                                    const DayCounter& dayCounter,
                                    const std::vector<Handle<Quote> >& jumps,
                                    const std::vector<Date>& jumpDates,
                                    const T& interpolator)
    : FxTermStructure(settlementDays, calendar, dayCounter, jumps, jumpDates),
      InterpolatedCurve<T>(interpolator) {}

    template <class T>
    InterpolatedFxCurve<T>::InterpolatedFxCurve(
                                 const std::vector<Date>& dates,
                                 const std::vector<Real>& rates,
                                 const DayCounter& dayCounter,
                                 const Calendar& calendar,
                                 const T& interpolator)
    : FxTermStructure(dates.front(), calendar, dayCounter),
      InterpolatedCurve<T>(std::vector<Time>(), rates, interpolator),
      dates_(dates)
    {
        initialize();
    }

    template <class T>
    InterpolatedFxCurve<T>::InterpolatedFxCurve(
                                 const std::vector<Date>& dates,
                                 const std::vector<Real>& rates,
                                 const DayCounter& dayCounter,
                                 const T& interpolator)
    : FxTermStructure(dates.front(), Calendar(), dayCounter),
      InterpolatedCurve<T>(std::vector<Time>(), rates, interpolator),
      dates_(dates)
    {
        initialize();
    }

    template <class T>
    void InterpolatedFxCurve<T>::initialize()
    {
        QL_REQUIRE(dates_.size() >= T::requiredPoints,
                   "not enough input dates given");
        QL_REQUIRE(this->data_.size() == dates_.size(),
                   "dates/data count mismatch");

        this->times_.resize(dates_.size());
        this->times_[0] = 0.0;
        for (Size i=1; i<dates_.size(); ++i) {
            QL_REQUIRE(dates_[i] > dates_[i-1],
                       "invalid date (" << dates_[i] << ", vs "
                       << dates_[i-1] << ")");
            this->times_[i] = dayCounter().yearFraction(dates_[0], dates_[i]);
            QL_REQUIRE(!close(this->times_[i],this->times_[i-1]),
                       "two dates correspond to the same time "
                       "under this curve's day count convention");
        }

        this->interpolation_ =
            this->interpolator_.interpolate(this->times_.begin(),
                                            this->times_.end(),
                                            this->data_.begin());
        this->interpolation_.update();
    }

}

#endif