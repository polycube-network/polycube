import os
import argparse
from converter import  create_junit_test_file, write_junit_file

parser = argparse.ArgumentParser(description='Test File')
parser.add_argument('test_suite_name', type=str, help='Class of tests to parse')
args = parser.parse_args()
OUTPUT_FILE = 'output.xml'
TESTS_FILE = os.environ['TEST_RESULTS']

if __name__ == "__main__":
   tests = create_junit_test_file(TESTS_FILE)
   write_junit_file(OUTPUT_FILE,tests,args.test_suite_name)