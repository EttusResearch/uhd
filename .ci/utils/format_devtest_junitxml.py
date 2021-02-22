from junitparser import JUnitXml, Element, Attr, TestCase
import argparse
import glob, os

class ClassNameTestCase(TestCase):
    classname = Attr('classname')

parser = argparse.ArgumentParser()
parser.add_argument("search_path")
parser.add_argument("output_name")
args = parser.parse_args()

xml = JUnitXml()
for file in glob.glob(args.search_path + "/**/*.xml", recursive=True):
    xml += JUnitXml.fromfile(file)

for suite in xml:
    for case in suite:
        classname_case = ClassNameTestCase.fromelem(case)
        if classname_case.name == 'test_all':
            classname_case.name = classname_case.classname
xml.write(args.output_name)
