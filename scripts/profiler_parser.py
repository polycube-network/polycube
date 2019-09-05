#!/usr/bin/env python

"""
Utility script to parse output from Profiler framework and print statistics.

More information about the benchmarking framework can be found in the
Polycube documentation, section 'developers/profiler.rst'.

This is a script with the aim of printing in a more human-readable format
information previously gathered by the framework.

Usage: profiler_parse.py <filename> [<FLAGS>]
    -h/--help: show the usage menu
    -s/--statistics: show only general statistics
    -e/--export: export in csv format
"""

import sys
import re
import csv

#Support variable
data = []
previous = 0
padding = 40

#Pattern to match. The framework's output file is:
#<filename>,<line_of_code>:<timestamp>
pattern = '(\\S+),(\\d+),(.*),(\\d+)'

#Function to parse a matched line
def parseMatch(match, number):
  global data, padding, previous
  #Used last string from '/' since the __FILE__ macro in C
  #could contain the entire path
  file = match.group(1)[match.group(1).rfind('/')+1:]
  line = int(match.group(2))
  name = match.group(3)
  loopId = len([1 for x in data if x[0] == file and x[1] == line])
  timestamp = int(match.group(4))
  duration = 0 if number == 0 else timestamp - previous
  previous = timestamp
  padding = padding if len(file)+len(str(line))+1 < padding else len(file)+len(str(line))+1
  padding = padding if len(name) < padding else len(name)
  data.append((file,line,name,loopId,timestamp,duration))

#Function to read the specified file
def readFile(filename):
  try:
    lines = open(filename, 'r').readlines()
    if len(lines) == 0:
        raise IOError()
  except IOError:
    print('Error: no such file \'' + sys.argv[1] + '\' or empty')
    exit()
  #Foreach line read, parse it and update data structures
  for line_number, line in enumerate(lines[1:]):
    match = re.match(pattern, line, flags=0)
    if match:
        parseMatch(match, line_number)
    else:
      print("Error while parsing line " + str(line_number) + ": " + line)
      print("Is it compliant with the format <file>,<line_of_code>,<checkpoint_name>,<timestamp> ?\n")
      showUsage()
      exit()

#Function to print script usage
def showUsage():
  print("profiler_parser.py <filename> [<FLAGS>]")
  print("\t-h/--help: show the usage menu")
  print("\t-s/--statistics: show only general statistics")
  print("\t-e/--export: export in csv format")

def main():
  global data

  #Checking arguments
  if len(sys.argv) == 1 or sys.argv[1] == "-h" or sys.argv[1] == "--help":
    showUsage()
    exit()

  readFile(sys.argv[1])

  #Checking export flag
  if len(sys.argv) == 3 and (sys.argv[2] == "--export" or sys.argv[2] == "-e"):
    filename = sys.argv[1][:sys.argv[1].rfind('.')]+".csv"
    with open(filename, 'w') as out:
      csv_out=csv.writer(out)
      csv_out.writerow(["File", "Line", "Name", "LoopId", "Timestamp(ns)", "Duration(ns)"])
      for row in data:
        csv_out.writerow(row)
    print("CSV exported at `" + filename + "`")
    exit()

  #Checking whether to display all point-to-point distances or not
  if len(sys.argv) == 2 or (len(sys.argv) == 3 and not (sys.argv[2] == "--statistics" or sys.argv[2] == "-s")):
    print("="*padding*3 + "="*6)
    print("||" + "FROM".ljust(padding) + "|" + "TO".ljust(padding) + "|" + "TIME(ns)".ljust(padding) + "||")
    print("="*padding*3 + "="*6)
    for count, item in enumerate(data[:-1]):
      name1 = item[2] if item[2] != "" else item[0] + "," +str(item[1])
      name2 = data[count+1][2] if data[count+1][2] != "" else data[count+1][0] + "," + str(data[count+1][1])
      print("||" + name1.ljust(padding) + "|" + name2.ljust(padding) + "|" + str(data[count+1][5]).ljust(padding) + "||")
    print("="*padding*3 + "="*6)

  #Printing statistics
  duration = [item[5] for item in data[1:]]
  print("Max execution time: " + str(max(duration)) + " ns")
  print("Min execution time: " + str(min(duration)) + " ns")
  print("Avg execution time: " + str(sum(duration)/len(duration)) + " ns")

if __name__== '__main__':
    main()