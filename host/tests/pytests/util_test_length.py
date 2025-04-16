from collections import namedtuple
import pytest

# This provides a way to run a quick smoke test run for PRs, a more exhaustive set
# of tests for nightly runs, and long running tests for stress tests over the weekend
#
# smoke:  subset of tests, short duration
# full:   all test cases, short duration
# stress: subset of tests, long duration
Test_Length_Smoke = "smoke"
Test_Length_Full = "full"
Test_Length_Stress = "stress"

test_length_params = namedtuple('test_length_params', 'iterations duration')

def select_test_cases_by_length(test_length, test_cases):
    if test_length == Test_Length_Full:
        return [test_case[1] for test_case in test_cases]
    else:
        return [test_case[1] for test_case in test_cases if test_length in test_case[0]]