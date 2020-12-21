from junitparser import JUnitXml
import sys

xml = JUnitXml.fromfile(sys.argv[1])
print(f"Total tests: {xml.tests}\nFailures: {xml.failures}")
print("Will end process with exit_code = number_of_failures")
exit(xml.failures)