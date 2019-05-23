Some hints for programmers
--------------------------

We apologize for this section being just an unstructured list of suggestions; better documentation will be created soon.

Install the provided git-hooks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

An interesting feature that git provides is the possibility to run custom scripts (called `Git Hooks`) before or after events such as: commit, push, and receive.
Git hook scripts are useful for identifying simple issues before submission to code review. We run our hooks on every commit to automatically point out issues in code such as missing semicolons, trailing whitespace, and debug statements.

To solve these issues and benefit from this feature, we use [pre-commit](https://pre-commit.com/), a framework for managing and maintaining multi-language pre-commit hooks.

The `.pre-commit-config.yaml` configuration file is already available under the root folder of this repo but before you can run hooks, you need to have the pre-commit package manager installed. You can install it using pip:

::

  sudo apt-get install python-pip -y
  pip install pre-commit

After that, run pre-commit install (under the project root folder) to install pre-commit into your git hooks. pre-commit will now run on every commit.

::

  pre-commit install

How to write a test
^^^^^^^^^^^^^^^^^^^

The following is a brief guideline about how to write tests for services. Please remember such tests are invoked by a more generic script that tries to execute all tests for all services and provide global results.

1. tests are placed under `pcn-servicename\test` folder (and its first level of subfolders).
E.g. `pcn-bridge\test` and `pcn-bridge\test\vlan` are valid folders.

2. tests name begins with `test*`

3. tests scripts must be executable (`chmod +x test.sh`)

4. never launch `polycubed`: polycubed is launched by the upper script, not in the script itself

5. exit on error: script should exit when a command fails (`set -e`)

6. tests must terminate in a fixed maximum time, no `read` or long `sleep` allowed

7. tests **must** exit with a **clean environment**: all `namespaces`, `links`, `interfaces`, `cubes` created inside the script must be destroyed when script returns.
In order to do that use a `function cleanup` and set `trap cleanup EXIT` to be sure cleanup function gets always executed (also if an error is encountered, and the script fails).

8. consider that when `set -e` is enabled in your script, and you want to check if, for instance, a `ping` or `curl` command succeeds, this check is implicitly done by the returning value of the command itself.
So `ping 10.0.0.1 -c 2 -w 4` makes your script succeed if ping works, and make your script fail if it doesn't.

9. if the test `succeded` it returns `0`, otherwise returns `non-zero-value` (this is the standard behavior).
In order to check a single test result, use `echo $?` after script execution to read return value.

Please refer to existing examples (E.g. [services/pcn-helloworld/test/test1.sh](services/pcn-helloworld/test/test1.sh))


Optimizing the compilation time of dataplane eBPF programs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The compilation time of dataplane eBPF programs is an important parameter in services that are composed by many eBPF programs and/or services that use the reloading capability, as this affect the time needed to apply your changes in the data path.

Unfortunately, including large headers in the datapath code has a noticeable impact on the compilation time. Hence, in some cases it is better to copy-paste some elements (struct, macro, function, etc) and definitions to avoid including the whole file, or move to more specific headers instead of including everything from the root headers folder.

The `pcn-firewall` service is an example of service that has been optimized in this way, decreasing the dynamic loading time from tens of seconds to a few seconds.


Additional hints
^^^^^^^^^^^^^^^^

1. **Creating multiple data plane programs**. If possible, it would be nice to create a single dataplane program, and enabling/disabling some portions using conditional compilation macros.

2. **Coding Style**: The `scripts/check-style.py` uses `clang-format` to check the code style.
This tool has to be executed from the root folder.
A list of files or folders to check is received as argument; the entire source code is checked when no parameters are passed.
The `--fix` option will automatically fix the code style instead of simply checking

3. **Trailing white spaces**: Trailing white spaces could generate some git noise.
Any decent text editor has an option to remove them automatically, it is a good idea to enable it.
Please notice that running `clang-format` will remove them automatically.
**NB**: If you are using our `pre-commit git hooks`, you do not need to remove the trailing whitespaces manually, they will be removed automatically at every commit.
If you want to remove them manually you can use execute the following commands in the polycube root folder, please note that this will remove trailing whitespaces of all files.

::

    find . -type f -name '*.h' -exec sed -i 's/[ \t]*$//' {} \+
    find . -type f -name '*.cc' -exec sed -i 's/[ \t]*$//' {} \+
    find . -type f -name '*.cpp' -exec sed -i 's/[ \t]*$//' {} \+

4. **Debugging using bpftool**: In some cases bpftool could be useful to inspect running eBPF programs and show, dump, modify maps values.

::

  #install
  sudo apt install binutils-dev libreadline-dev
  git clone https://github.com/torvalds/linux
  cd linux/tools
  make bpf
  cd bpf
  sudo make install

  #usage
  sudo bpftool

Continuous Integration
^^^^^^^^^^^^^^^^^^^^^^

A continuous integration system based on `jenkins` is set-up for this project.

`build` pipeline is in charge to re-build the whole project from scratch at each commit.
`testing` pipeline is in charge to run all repo system tests at each commit.

pipelines results are shown in `README` top buttons `build` and `testing`.

In order to get more info about tests results, click on such buttons to get redirected to jenkins environment, then click the build number to get detailed info about a specific commit.
`testing/build` results can be read opening the tab `Console Output`.

What to do when *build failing* ?
*********************************

Click in build failing button, go to jenkins control panel, open the build number is failing, open console output in order to understand where build is failing.

What to do when *testing failing* ?
***********************************

Click in testing failing button, go to jenkins control panel, open the build number is failing, open console output in order to understand where tests are failing. At the end of the output there is a summary with tests passing/failing.


Valgrind
^^^^^^^^

Valgrind is an open source tool for analyzing memory management and threading bugs. It can easily discover memory leaks, and spot possible segfault errors.


Requirements for polycubed: (1) valgrind 3.15+ (2) disable `setrlimit` in polycubed.cpp

1 Install valgrind 3.15
***********************

Valgrind 3.14+ is introducing support for bpf() system call.
Previous versions won't work.

- Download Valgrind 3.15+ source from here http://www.valgrind.org/downloads/current.html
- Build Valgrind from source http://valgrind.org/docs/manual/dist.install.html
::

    ./configure
    make
    sudo make install

2 Disable `setrlimit`
*********************

Only for debug purposes and in order to be able to run valgrind we have to disable `setrlimit` in polycubed.cpp.

We suggest to comment out following lines in :scm_web:`polycubed.cpp <src/polycubed/src/polycubed.cpp#L226>`
::

    // Each instance of a service requires a high number of file descriptors
    // (for example helloworld required 7), hence the default limit is too low
    // for creating many instances of the services.
    struct rlimit r = {32 * 1024, 64 * 1024};
    if (setrlimit(RLIMIT_NOFILE, &r)) {
        logger->critical("Failed to set max number of possible filedescriptor");
        return -1;
    }
