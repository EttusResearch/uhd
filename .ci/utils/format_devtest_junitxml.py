from junitparser import JUnitXml, Element, Attr, TestCase
import argparse
import glob, os

class ClassNameTestCase(TestCase):
    classname = Attr('classname')

parser = argparse.ArgumentParser()
parser.add_argument("search_path")
parser.add_argument("output_name")
parser.add_argument("--fpgas")
args = parser.parse_args()

if args.fpgas:
    return_xml = JUnitXml()
    for fpga in args.fpgas.split(','):
        xml = JUnitXml()
        for file in glob.glob(args.search_path + "/" + fpga + "/**/*.xml", recursive=True):
            xml += JUnitXml.fromfile(file)
        for suite in xml:
            for case in suite:
                classname_case = ClassNameTestCase.fromelem(case)
                if classname_case.name == 'test_all':
                        classname_case.name = fpga + " " + classname_case.classname
        return_xml += xml
    return_xml.write(args.output_name)

else:
    return_xml = JUnitXml()
    for file in glob.glob(args.search_path + "/**/*.xml", recursive=True):
        return_xml += JUnitXml.fromfile(file)

    for suite in return_xml:
        for case in suite:
            classname_case = ClassNameTestCase.fromelem(case)
            if classname_case.name == 'test_all':
                    classname_case.name = classname_case.classname

    return_xml.write(args.output_name)
