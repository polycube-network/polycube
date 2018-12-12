# Test scripts
All scripts create a number of random rules, based on the test name, just to test the computation time in the worst case (rules that are matched are the last), and some useful rules to be matched.

## Test2_xx
Test2_xx create one pbforwarder with xx rules based on the MAC addresses and executes two ping cyles.

## Test3_xx
Test3_xx create one pbforwarder with the first xx-2 rules all based on level 2 and 2 rules based on level 2.

## Test4_xx
Test3_xx create one pbforwarder with the first xx-2 rules all based on level 4 and 2 rules based on level 2.

## Test23_xx
Test23_xx create xx rules based some on level 2, some on level 3, some on both.

## Test234_xx
Test23_xx create xx rules based some on level 2, some on level 3, some on level 4, some complete.


