Polyube metrics with Prometheus
=================================

Test phase
-----------
We chose to use the `TDD <https://en.wikipedia.org/wiki/Test-driven_development>`_ approach.



Google-Test
-------------
As a library to write tests in C++ was chosen: `googletest <https://github.com/google/googletest.git>`_




Brief architecture
------------------
grafico slide




Prometheus-cpp
---------------
 The library chosen to create metrics was `Prometheus-cpp <https://github.com/jupp0r/prometheus-cpp.git>`_ which was added as a submodule to Polycube, modifying the gitsubmodules file and the CMakeLists.txt in polycube/src/polycubed/src (a comment for the future has also been added in the file scripts/pre-requirements.sh)



