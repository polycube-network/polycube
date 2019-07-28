# Scripts for Continuous Integration

This folder contains the scripts that are used by Polycube continuous integration system, currently based on Jenkins.

 - ``JenkinsfileTestBuild``: checks if the build script works.
 - ``JenkinsfileTestCleanInstance``: builds Polycube and runs all tests (both the ones related to the framework and the ones specific for each individual service). Polycubed is restarted at the end of each test.
 - ``JenkinsfileTestSameInstance``: builds Polycube and runs all tests (both the ones related to the framework and the ones specific for each individual service). Polycubed is never restarted; a single instance runs all tests.
 - ``JenkinsfileTestIptables``:  builds pcn-iptables and runs all tests related to this component.

