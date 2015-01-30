import PythonModule as pm
import xml.etree.ElementTree as ET

#Parse
QL = pm.QL('TEST QL')
doc = ET.XML(QL.calc())

print "tag=" + doc.tag
print "text=" + doc.text

#Create
root = ET.ElementTree(doc)
xml_input = ET.tostring(doc, encoding='utf8', method='xml')

QL = pm.QL(xml_input)
print QL.calc()


a = input("press Enter")
