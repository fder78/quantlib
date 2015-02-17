
#define NEW_VERSION_
#ifdef NEW_VERSION_


#include <ql/quantlib.hpp>
#include <boost/timer.hpp>
#include <Windows.h>

using namespace QuantLib;

// helper function that prints the result of a model calibraiton to std::cout
void printModelCalibration(
    const std::vector<boost::shared_ptr<CalibrationHelper> > &basket,
    const Array &volatility) { //model sigma

    std::cout << "\n" << std::left << std::setw(20) << "Expiry" << std::setw(14)
              << "Model sigma" << std::setw(20) << "Model price"
              << std::setw(20) << "market price" << std::setw(14)
              << "Model ivol" << std::setw(14) << "Market ivol" << std::fixed
              << std::setprecision(4) << std::endl;
    std::cout << "====================================================================================================" << std::endl;

    for (Size j = 0; j < basket.size(); ++j) {
        boost::shared_ptr<SwaptionHelper> helper =
            boost::dynamic_pointer_cast<SwaptionHelper>(basket[j]);
        Date expiry = helper->swaption()->exercise()->date(0);
        std::ostringstream expiryString;
        expiryString << expiry;
        std::cout << std::setw(20) << expiryString.str() << std::setw(14)
                  << volatility[j] << std::setw(20) << basket[j]->modelValue()
                  << std::setw(20) << basket[j]->marketValue() << std::setw(14)
                  << basket[j]->impliedVolatility(basket[j]->modelValue(), 1E-6,
                                                  1000, 0.0, 2.0)
                  << std::setw(14) << basket[j]->volatility()->value()
                  << std::endl;
    }
    if (volatility.size() > basket.size()) // only for markov model
        std::cout << std::setw(20) << " " << volatility.back() << std::endl;
}

// here the main part of the code starts
int main(int argc, char *argv[]) {

    try {

		system("mode con: cols=120");
        std::cout << "\nGaussian1dModel Examples" << std::endl;

        boost::timer timer;
		//evaluation date
        Date refDate(30, April, 2014);
        Settings::instance().evaluationDate() = refDate;
		//forward rate & discount rate
        Real forward6mLevel = 0.025;
        Real oisLevel = 0.02;
        Handle<Quote> forward6mQuote(boost::make_shared<SimpleQuote>(forward6mLevel));
        Handle<Quote> oisQuote(boost::make_shared<SimpleQuote>(oisLevel));
        Handle<YieldTermStructure> yts6m(boost::make_shared<FlatForward>(0, TARGET(), forward6mQuote, Actual365Fixed()));
        Handle<YieldTermStructure> ytsOis(boost::make_shared<FlatForward>(0, TARGET(), oisQuote, Actual365Fixed()));
        boost::shared_ptr<IborIndex> euribor6m = boost::make_shared<Euribor>(6 * Months, yts6m);
		//swaption vol
        Real volLevel = 0.20;
        Handle<Quote> volQuote(boost::make_shared<SimpleQuote>(volLevel));
        Handle<SwaptionVolatilityStructure> swaptionVol(boost::make_shared<ConstantSwaptionVolatility>(0, TARGET(), ModifiedFollowing, volQuote, Actual365Fixed()));
		//bermudan swaption desc. 10Y
        Real strike = 0.02;
        Date effectiveDate = TARGET().advance(refDate, 2 * Days);
        Date maturityDate = TARGET().advance(effectiveDate, 10 * Years);

        Schedule fixedSchedule(effectiveDate, maturityDate, 1 * Years, TARGET(), ModifiedFollowing, ModifiedFollowing, DateGeneration::Forward, false);
        Schedule floatingSchedule(effectiveDate, maturityDate, 6 * Months, TARGET(), ModifiedFollowing, ModifiedFollowing, DateGeneration::Forward, false);

        boost::shared_ptr<NonstandardSwap> underlying = boost::make_shared<NonstandardSwap>(VanillaSwap(
                VanillaSwap::Payer, 1.0, fixedSchedule, strike, Thirty360(), floatingSchedule, euribor6m, 0.0, Actual360()));

        std::vector<Date> exerciseDates;
        for (Size i = 1; i < fixedSchedule.size()-1; ++i)
            exerciseDates.push_back(TARGET().advance(fixedSchedule[i], -2 * Days));

        boost::shared_ptr<Exercise> exercise = boost::make_shared<BermudanExercise>(exerciseDates, false);
        boost::shared_ptr<NonstandardSwaption> swaption = boost::make_shared<NonstandardSwaption>(underlying, exercise);
		boost::shared_ptr<SwapIndex> swapBase = boost::make_shared<EuriborSwapIsdaFixA>(10 * Years, yts6m, ytsOis);
		
		Real reversion = 0.01;
		std::cout<<"(9) Markov Functional"<<std::endl;
        std::vector<Date> markovStepDates(exerciseDates.begin(), exerciseDates.end());
        std::vector<Date> cmsFixingDates(markovStepDates);
        std::vector<Real> markovSigmas(markovStepDates.size() + 1, 0.01);
        std::vector<Period> tenors(cmsFixingDates.size(), 10 * Years);
        boost::shared_ptr<MarkovFunctional> markov = boost::make_shared<MarkovFunctional>(
                yts6m, reversion, markovStepDates, markovSigmas, swaptionVol, cmsFixingDates, tenors, swapBase, MarkovFunctional::ModelSettings().withYGridPoints(16));

        boost::shared_ptr<Gaussian1dSwaptionEngine> swaptionEngineMarkov =
            boost::make_shared<Gaussian1dSwaptionEngine>(markov, 8, 5.0, true, false, ytsOis);
        boost::shared_ptr<Gaussian1dFloatFloatSwaptionEngine> floatEngineMarkov =
            boost::make_shared<Gaussian1dFloatFloatSwaptionEngine>(markov, 16, 7.0, true, false, Handle<Quote>(), ytsOis, true);

		

		boost::shared_ptr<FloatFloatSwap> underlying4 = boost::make_shared<FloatFloatSwap>(
			VanillaSwap::Payer, 1.0, 1.0, fixedSchedule, swapBase, Thirty360(), floatingSchedule, euribor6m, Actual360());
		boost::shared_ptr<FloatFloatSwaption> swaption4 = boost::make_shared<FloatFloatSwaption>(underlying4, exercise);
        swaption4->setPricingEngine(floatEngineMarkov);

  //      timer.restart();
  //      Real npv7 = swaption4->NPV();
  //      std::cout << "\nFloat swap NPV (Markov) = " << std::setprecision(6) << swaption4->result<Real>("underlyingValue") << std::endl;
		//std::cout<<"calc time = " << timer.elapsed() << "s" <<std::endl;	

		//////////////////////////////////////////////////////////////////////////////////////
		std::vector<Date> stepDates(exerciseDates.begin(), exerciseDates.end() - 1);
		std::vector<Real> sigmas(stepDates.size() + 1, 0.01);
		boost::shared_ptr<Gsr> gsr = boost::make_shared<Gsr>(yts6m, stepDates, sigmas, reversion);
		boost::shared_ptr<PricingEngine> swaptionEngine = boost::make_shared<Gaussian1dSwaptionEngine>(gsr, 64, 7.0, true, false, ytsOis);
		boost::shared_ptr<PricingEngine> nonstandardSwaptionEngine = boost::make_shared<Gaussian1dNonstandardSwaptionEngine>(gsr, 64, 7.0, true, false, Handle<Quote>(), ytsOis);

		swaption->setPricingEngine(nonstandardSwaptionEngine);

		LevenbergMarquardt method;
		EndCriteria ec(1000, 10, 1E-8, 1E-8, 1E-8); // only max iterations use actually used by LM
		
		std::vector<boost::shared_ptr<CalibrationHelper> > basket = swaption->calibrationBasket(swapBase, *swaptionVol, BasketGeneratingEngine::Naive);
        for (Size i = 0; i < basket.size(); ++i)
            basket[i]->setPricingEngine(swaptionEngineMarkov);

        timer.restart();
        markov->calibrate(basket, method, ec);
        printModelCalibration(basket, markov->volatility());
        std::cout<<"calc time = " << timer.elapsed() << "s" <<std::endl;	

        timer.restart();
        Real npv8 = swaption4->result<Real>("underlyingValue");        
        std::cout << "\nFloat swap NPV (Markov) = " << std::setprecision(6) << npv8 << std::endl;
		std::cout<<"calc time = " << timer.elapsed() << "s" <<std::endl;	
    }
    catch (QuantLib::Error e) {
        std::cout << "terminated with a ql exception: " << e.what()
                  << std::endl;
        return 1;
    }
    catch (std::exception e) {
        std::cout << "terminated with a general exception: " << e.what()
                  << std::endl;
        return 1;
    }
}



#endif