"""
Source Generator Plugins for RF Test Scripts
"""

import importlib
from inspect import isclass
from builtins import input, object
from six import itervalues


###############################################################################
# Source Generator Plugins
###############################################################################
class SourceGenerator(object):
    """
    Parent class for source generators.
    """
    def __init__(self, log, **_):
        self.log = log

    def tune(self, freq, power_lvl_dbm):
        """
        Set the TX frequency. This function can block until the Tx frequency is
        stable, so if LOs need to settle or whatnot, it's OK for this function
        to take a while to return. After it's returned, the assumption is that
        we can start RXing.
        """
        raise NotImplementedError()

    def tear_down(self):
        """Do all necessary clean-up for the source generator"""
        pass


class ManualSourceGenerator(SourceGenerator):
    """
    Not really a source generator, but a command-line interaction with the user
    to manually set the TX source.
    """
    def tune(self, freq, power_lvl_dbm):
        """
        Ask the user to set the TX frequency. Once the user continues, the
        assumption is that we can start RXing.
        """
        input("Please tune the signal generator to {:.3f} MHz and {:.1f} dBm, "
              "then press Enter".format(freq / 1e6, power_lvl_dbm))


DEFAULT_SOURCE_GENERATORS = {
    'default': ManualSourceGenerator,
}
DEFAULT_SOURCE_GENERATOR = 'default'


def get_source_generator(log=None, src_gen_id=None, **kwargs):
    """
    Factory function to get a source generator
    """
    src_gen_id = src_gen_id or DEFAULT_SOURCE_GENERATOR
    if src_gen_id in DEFAULT_SOURCE_GENERATORS:
        return DEFAULT_SOURCE_GENERATORS.get(src_gen_id)(log, **kwargs)
    try:
        module = importlib.import_module(src_gen_id)
    except ImportError:
        raise RuntimeError("Could not find source generator plugin `{}'!".format(
            src_gen_id))
    src_gens = [
        x for x in itervalues(module.__dict__)
        if isclass(x) and issubclass(x, SourceGenerator) and x != SourceGenerator
    ]
    if not src_gens:
        raise RuntimeError(
            "Could not find any source generator classes in module `{}'!"
            .format(src_gen_id))
    if len(src_gens) > 1:
        raise RuntimeError(
            "Ambiguous source generator plugin `{}'! Too many generator classes: {}"
            .format(src_gen_id, src_gens))
    return src_gens[0](log, **kwargs)
