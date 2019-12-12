import re
from junit_xml import TestSuite, TestCase
import itertools



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

def create_junit_test_file(tests_file:str):
   tests = []
   for file in tests_file.split("\n"):
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
            if test_name in test:
               tc = TestCase(name=test_name, elapsed_sec=int(duration),status=status.split("+")[0], stdout=test[test_name])
               if status != "Passed++++":
                  tc.add_failure_info(output=test[test_name])
            else:
               tc = TestCase(name=test_name, elapsed_sec=int(duration), status=status.split("+")[0])
            tests.append(tc)
   return tests

def write_junit_file(output_file:str, tests:[], test_name:str) -> None:
   # We create a TestSUite and we write them in a XML File
   t = TestSuite(name=test_name,test_cases=tests)
   with open('output.xml', 'w') as f:
      TestSuite.to_file(f, [t], prettyprint=False)

