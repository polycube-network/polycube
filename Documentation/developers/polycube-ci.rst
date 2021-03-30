Polycube CI/CD
==============

This guide is the starting point for developers that want to deal with CI/CD processes based on GitHub Actions.


Introduction
------------
GitHub Actions help you automate tasks within your software development life cycle by means of workflows.

The workflow is an automated procedure that you add to your repository inside the .github/workflows/ directory. Are made up of one or more jobs and can be scheduled or triggered by an event.

The workflow can be used to build, test, package, release, and deploy a project on GitHub.


Implementation with polycube
----------------------------
You can find the workflow for CI/CD under .github/workflows/ci.yml.

This workflow can start with three different events for three different tasks:

  1.  PR opened/synchronized/reopened
  2.  Commit push on master (PR accepted)
  3.  Tag push on master (tag with format "v*")

The first event is used to build and test any open PRs, then the workflow assumes a "dev" state for its entire duration.

The second workflow is used to build and push an updated version of polycube on docker after a PR merge. In this situation the status of the workflow assumes the value of "master".

The third event is used to build and push a polycube release on docker with an official tag. The status of the workflow becomes "release" and a changelog is also published on github starting from the last release.


Workflow in "dev" state
-------------------------
This workflow is used to validate any open PR for this reason all tests are performed after the build. If there is only one failed test, the entire workflow will end in error, reporting it via annotation. Furthermore, it is possible to find the logs of the tests through the artifact section within the page of the performed workflow.
In this state only the full polycube image will be built and published on **Docker Hub** under the ``polycubenets/polycube-pr`` repository.


Workflow in "master" state
----------------------------
Once validated and accepted a PR this workflow start in order to updated the latest version of polycube by publishing the new image on ``polycubenets`` repository.
Here the individual names change to polycube, polycube-iptables and polycube-k8s.


Workflow in "release" state
-----------------------------
This workflow is used to release an official polycube tag. The new image will always be built and published on the ``polycubenets`` repository with the official names (polycube, polycube-iptables and polycube-k8s).
It also provides for the creation of a release on GitHub by generating a changelog that summarizes all the accepted PRs starting from the previous release.


