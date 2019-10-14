import re
from junit_xml import TestSuite, TestCase
import itertools
import os
import argparse
import os

parser = argparse.ArgumentParser(description='Test File')
parser.add_argument('test_suite_name', type=str, help='Class of tests to parse')
args = parser.parse_args()
TESTS_FILE = os.environ['TEST_RESULTS']

tests = []

def file_log_parser(file:str) -> dict:
   with open(file) as file_log:
      config = file_log.read()
      test_logs_results = {}
      index = ""
      for line in config.split("\n"):
         if "++++Starting test" in line:
            index = line.split(" ")[-1][:-4]
            test_logs_results[index] = ""
         elif "++++TEST" in line:
            index = ""
         elif index:
            test_logs_results[index] += line + "\n"
   return test_logs_results
            
for file in TESTS_FILE.split("\n"):
   file_log = "test_log_" + "_".join(file.split("_")[2:])
   test = file_log_parser(file_log)
   with open(file) as file_buffer: # Use file to refer to the file object
      config = file_buffer.read()
      current_section = {}
      lines = config.split()
      date = lines[1]
      is_relaunch = lines[2]
      debug=lines[3]
      for _,test_name,status,_,_,duration in zip(*[iter(lines[6:])]*6):
         if duration == "false":
            continue
         else:
            duration=duration[:-1]
         tc = TestCase(name=test_name, elapsed_sec=int(duration),status=status)
         if status != "PASSED++++":
            print(len(test[test_name]))
            tc.add_failure_info(output=test[test_name])
         tests.append(tc)
t = TestSuite(name=args.test_suite_name,test_cases=tests)
with open('output.xml', 'w') as f:
   TestSuite.to_file(f, [t], prettyprint=False)

from junitparser import JUnitXml

xml1 = JUnitXml.fromfile('output.xml')
print(xml1)