import sys
import xmltodict

xml_file = open(sys.argv[1])
dict_file = xmltodict.parse(xml_file.read())
test_suites = dict_file['testsuites']
test_suite = test_suites['testsuite']

# Find tests not passed
not_passed = 0
for test_case in test_suite['testcase']:
    if test_case['@status'] != 'Passed':
        not_passed += 1
        print(f"::error file=check_tests.py,line=14,col=1::Test {test_case['@status']}: {test_case['@name']}")

if not_passed > 0:
    print(f"::warning file=check_tests.py,line=17,col=1::Total tests not passed {not_passed } on {test_suites['@tests']}")
else:
    print("All tests passed")

exit(not_passed)
