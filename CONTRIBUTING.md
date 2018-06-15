# Contributing to UHD

## Reporting Bugs and other Issues

**Please don't use the bug/issue tracker to ask questions. If you have
questions, please ask on the usrp-users mailing list, or email
support@ettus.com!**

If you have found an issue or bug in UHD, you can use the
[GitHub issue tracker](https://github.com/EttusResearch/uhd/issues) to submit a
report. Please [be as specific as possible](#how-to-submit-a-bug-report), and
use the [issue template](.github/ISSUE_TEMPLATE.md), it will be easier for us to
debug an issue if we have all the information it asks for.

Before you submit a bug report, please make sure the issue you're experiencing
is not already solved. The following resources might help you fix your issue:

- [The UHD/USRP Manual](https://files.ettus.com/manual/). Some devices have
  "troubleshooting" or "known issues" sections.
- The [USRP users mailing list][ettus-ml], ([searchable archive][mailarchive])
- [The Ettus Knowledge Base](https://kb.ettus.com/Knowledge_Base)

Also, please confirm there's no other issue already open which describes your
issue.

### How to submit a bug report

When submitting a bug report, please provide all the information required to
reproduce the issue, such as:

- Exact UHD version (git hash, if you can)
- Which devices you're seeing this problem on
- How are the devices connected (network, PCIe, clocking, RF, ...)

Most importantly, provide a command or program that we can run to reproduce the
issue. The examples shipped with UHD are good starting points. Remember that the
issue tracker on GitHub is public, so don't post any sensitive or classified
information there.


## Code Contributions

If you are interested in submitting bug fixes or enhancements, please read the
following sections. Please note that we will reserve the right to reject any
kind of submission. Therefore, we advise getting in touch with us before doing
major work.

### Legal Notes

All code in this repository, unless specified otherwise, is copyright of
Ettus Research / National Instruments. For any non-trivial contribution, we
require a signed [Contributor License Agreement (CLA)][ettus-cla] for every
developer submitting code to assign copyright of contributions over to
Ettus Research / National Instruments.

Small changes (less than 10 lines of code), or documentation fixes, may be
accepted without prior submission of a CLA. This decision is made by
Ettus Research / National Instruments.

### Coding Guidelines

The following guidelines apply:

- [UHD Coding Guidelines][uhd-coding] (for anything in this repository)
- [FPGA Coding Guidelines][fpga-coding] (for any FPGA-related modifications)

[fpga-coding]: https://github.com/EttusResearch/fpga/blob/master/CODING.md
[uhd-coding]: https://github.com/EttusResearch/uhd/blob/master/CODING.md
[ettus-cla]: http://files.ettus.com/licenses/Ettus_CLA.pdf
[ettus-ml]: http://lists.ettus.com/mailman/listinfo/usrp-users_lists.ettus.com
[mailarchive]: https://www.mail-archive.com/usrp-users@lists.ettus.com

