import PythonModule as pm
import os
import xml.etree.ElementTree as ET

#Parse
input = '''<root><param_root><data_root><record><Group type="double" value="2"></Group><Name type="string" value="IRSFICC445"></Name><SimpleName type="string" value="IRSFICC445"></SimpleName><TraderID type="string" value="TJYI"></TraderID><Code type="string" value="IRSFICC445"></Code><Book type="string" value="IRS_KRW"></Book><BacktoBack type="string" value="Structured"></BacktoBack><ExtCalculator type="string" value="IRSFICC445"></ExtCalculator><CounterParty type="string" value="Meritz"></CounterParty><Calc type="string" value="Y"></Calc><CalcDelta type="string" value="Y"></CalcDelta><CalcChunkDelta type="string" value="Y"></CalcChunkDelta><CalcVega type="string" value="N"></CalcVega><Type type="string" value="VanillaSwap"></Type><PayRec type="string" value="Payer"></PayRec><TradeDate type="date" value="2/8/2013"></TradeDate><Upfrontpayment type="double" value="0"></Upfrontpayment><EvalDate type="date" value="2/8/2013"></EvalDate><Notional type="double" value="15000000000"></Notional><Currency type="string" value="KRW"></Currency><DiscountRateCurve type="string" value="KRW"></DiscountRateCurve><Obs1Ticker type="string" value="KWCDC Curncy"></Obs1Ticker><PaymentCalendar type="string" value="Seoul"></PaymentCalendar><EffectiveDate type="date" value="2/12/2013"></EffectiveDate><TerminationDate type="date" value="2/12/2015"></TerminationDate><Tenor type="string" value="Quarterly"></Tenor><Calendar type="string" value="Seoul"></Calendar><BDC type="string" value="Following"></BDC><BDC_Terminal type="string" value="Following"></BDC_Terminal><Rule type="string" value="Backward"></Rule><EOM type="double" value="0"></EOM>				<FirstDate type="date" value="2/12/2013"></FirstDate><NextToLastDate type="date" value="2/12/2015"></NextToLastDate><FixedRate type="double" value="0.02735"></FixedRate><FixedDayCounter type="string" value="ActualActual(ISMA)"></FixedDayCounter><FloatingRate type="string" value="CD91"></FloatingRate><FloatingSpread type="double" value="0"></FloatingSpread><FloatingDayCounter type="string" value="ActualActual(ISMA)"></FloatingDayCounter><RefCurve type="string" value="KRW"></RefCurve></record></data_root><eval_time type="date" value="1/23/2015"></eval_time><data_date_alias type="double" value="0"></data_date_alias></param_root><func_root name="RunPricing"></func_root></root>'''

QL = pm.QL(input)
#doc = ET.XML(QL.calc())

#print "tag=" + doc.tag
#print "text=" + doc.text

#Create

# root = ET.ElementTree(doc)
# xml_input = ET.tostring(doc, encoding='utf8', method='xml')
#
# QL = pm.QL(xml_input)
print QL.calc()

os.system('Pause')
