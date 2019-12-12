from converter import  create_junit_test_file, write_junit_file
import assertpy

# content of test_sample.py

def test_count_single_file_1():
    t = create_junit_test_file("test_results_1.txt")
    assertpy.assert_that(len(t)).is_equal_to(10)

def test_count_single_file_2():
    t = create_junit_test_file("test_results_1.txt\ntest_results_2.txt")
    assertpy.assert_that(len(t)).is_equal_to(29)

def test_assert_status_1():
    t = create_junit_test_file("test_results_1.txt")
    for test in t:
        assertpy.assert_that(test.status).is_equal_to("Failed")

def test_assert_status_2():
    t = create_junit_test_file("test_results_2.txt")
    for test in t:
        assertpy.assert_that(test.status).is_equal_to("Passed")

def test_failure_1():
    t = create_junit_test_file("test_results_1.txt")
    assertpy.assert_that(len(t)).is_equal_to(10)
    for test in t:
        assertpy.assert_that(test.is_failure()).is_not_none()

def test_not_failure_1():
    t = create_junit_test_file("test_results_2.txt")
    assertpy.assert_that(len(t)).is_equal_to(19)
    for test in t:
        assertpy.assert_that(test.is_failure()).is_none()