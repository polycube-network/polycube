Profiler Framework
^^^^^^^^^^^^^^^^^^

Simple, easy to use and fast C++ header class framework for profiling and benchmark.

Introduction
************

This framework provides an efficient and simple benchmarking primitive for user programs. Particularly, this can be used to profile the control plane portion of Polycube.
Although there are several powerful tools (particularly in Linux) that target benchmarking and profiling, they are often difficult to learn for an occasional user, which may need to perform some simple and fast benchmarking.

A worth-mentioning tool is  `perf <https://perf.wiki.kernel.org/index.php/Main_Page>`_.
Perf is a Linux profiling tool with performance counters which helps users to understand at low-level how not only the program, but also the hardware behaves. As the wiki reports, this tool not only is quite complex to learn, but it also produces deep information that users may not use or that they have to aggregate them to achieve a readable result. Moreover, perf refers to different types of event (ex. I/O disk) users may not be interested in.

This library provides a simple to use method not only to run a whole program benchmark, but also to measure single code-fragments performance.


How it works
************

The framework defines two different main components:

* Check-point: ``CHECKPOINT(<name>)``
* Store-point: ``STOREPOINT(<name>)``

A check-point is a point of the code where we simply want to take a measure (timestamp).

A store-point is a specialized check-point which not only acts like it, but it also manages the data storing into a file.

While the usage of a check-point leads to a negligible overhead (\~60 nanoseconds), it is suggested to use wisely the store-points, since the I/O operation on file are more expensive.

Each measure is characterized by:

- filename: the name of the file which contains the check-point;
- line of code: the line of code where the check-point has been inserted;
- name (optional): the name/description of the checkpoint given by the user;
- timestamp: the measure in *nanoseconds* taken.

Accordingly, the format used to store check-points is: `<filename>,<line_of_code>,<name>,<timestamp>`.

The output file would be available at ``/var/log/polycube/profile_<date>_<time>).log`` where date and time refer to execution moment.

Usage
*****

The framework is turned on by defining ``#define PROFILER`` in your code (wherever the user wants) but **before**  including the profiling header (i.e., `#include <polycube/profile.h>`). Without this definition, the framework will be automatically turned off for all the execution.

Possible operations:

* to insert Check-points, the user have just to insert the macro ``CHECKPOINT(<name>)``.
* to insert Store-points, insert the macro ``STOREPOINT(<name>)``.
* to remove them, comment the proper line or remove it.

An interesting feature is that in case the user deactivates the framework, it is not required to remove every checkpoints he had previously inserted in the code: the framework will handle it by replacing them with an empty line.

The output file created by the framework can be easily imported by every spreadsheet program (LibreOffice, Excel,...), but since it does not contain clear data, the user can use the script ``profiler_parser.py`` (Doc. inside the script) to achieve a more complete, elegant and importable as well result.

Example
*******

In this simple program we want to measure the performance of our function to compute the square of a number. We insert two check-points between the functions call and a final store-point at the end.

.. code-block:: c++
 
  #define PROFILER
  #include <polycube/profiler.h>
  
  int main(int argc, char **argv) {
    CHECKPOINT("Beginning")                 //line 7
    computeSquare(1);
    CHECKPOINT("After first computation")   //line 9
    computeSquare(2);
    STOREPOINT("After second computation")  //line 11
    return 0;
  }

After the execution, we obtain the following ``profile_20190902_181847.log`` file:

.. code-block:: text

  main.cpp,7,Beginning,1567333232104876595
  main.cpp,9,After first computation,1567333232104876666
  main.cpp,11,After second computation,1567333232104876737

Since the stored timestamps refer to the start of the epoch, to beautify the data and print some statistics we use our script ``profiler_parser.py``:


.. code-block:: bash

  $ python profiler_parser.py profile_20190902_181847.log

 ========================================================================
 ||FROM                   |TO                      |TIME(ns)           ||
 ========================================================================
 ||Beginning              |After first computation |71                 ||
 ||After first computation|After second computation|71                 ||
 ========================================================================
 Max execution time: 71 ns
 Min execution time: 71 ns
 Avg execution time: 71.0 ns

In case some checkpoint has not been given the name, the displayed information will follow the format
``<file>,<line>`` in order to identify the checkpoint as well. The script has another feature available in case we
would like to export data in .csv format: it can detect similar checkpoints inside a loop by comparing all the
available information and it assigns them an additional identifier to clarify the differences among them.

Performance analysis
********************

For this analysis, tests have been run on a Dell Xps 9570. Please consider that results are different depending on your processors and the available CPU.

The test has been run on a simple file where we inserted 100 consecutively check-points and a final store-point. The check-points are inserted inside a loop and there is not any other function call except for the check-points themselves. The aim of this program is to measure the time taken by each `CHECKPOINT()` to perform and save the single measurement. The final store-points has not been taken into account in the performance analysis result, since the processor's BPU (branch predictor unit) would probably fail the prediction of the last loop cycle (which has to exit the loop), leading to some small delay.

.. code-block:: bash

 Max execution time: 69 ns
 Min execution time: 46 ns
 Avg execution time: 49.515151515151516 ns

These results are really significant and they prove that the overhead added by the framework is small and does not interfere with the performance of calling program.
