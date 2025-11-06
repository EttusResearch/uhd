import pytest
import uhd.rfnoc

def pytest_addoption(parser):
    parser.addoption(
        "--args",
        type=str,
        nargs='?',
        required=True,
        help="arguments",)

@pytest.fixture(scope="session")
def graph(request):
    args = request.config.getoption("--args")
    return uhd.rfnoc.RfnocGraph(args)
