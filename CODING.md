# UHD Coding Standards

Note: This file applies to all code within the UHD repository, not just for
code that is actually part of the UHD library.

## Preamble

To quote R. W. Emerson: "A foolish consistency is the hobgoblin of little minds,
adored by little statesmen and philosophers and divines". Ignoring the little
statesmen for a minute, these coding standards are here to make our life
*easier*, not simply add additional rules. They are meant as additional guidance
for developers, and are not meant to be interpreted as law.

So, ultimately, it is up to the developer to decide how much these guidelines
should be heeded when writing code, and up to reviewers how much they are
relevant to new submissions.
That said, a consistent codebase is easier to maintain, read, understand, and
extend. Choosing personal preferences over these coding guidelines is not a
helpful move for the team and future maintainability of the UHD codebase.

## General Coding Guidelines

* Never submit code with trailing whitespace.
* Code layout: We use 4 spaces for indentation levels, and never tabs.
* Code is read more often than it's written. Code readability is thus something
  worth optimizing for.
* Try and keep line lengths to 79 characters, unless readability suffers. We
  often have to do fun things like SSH into machines and edit code in a
  terminal, and do side-by-side views of code on small-ish screens, so this is
  actually pretty helpful.
* Go crazy with log messages. Trace-level log messages in particular can be
  used copiously and freely (unless in rare cases where the can interfere with
  performance). Note that in C++, we have the option of fully compiling out
  trace-level messages (and even higher levels).

## C++-specific Guidelines

* If in doubt, consult the [C++ Core Guidelines][https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md].
  If the guidelines have an answer, and it works for you, just pick that.
* Use Doxygen doc-blocks copiously.
* All things equal, prefer standard C++ constructs over Boost constructs (see
  also Boost guidelines).
* Given the option, prefer C++ lambdas over std::bind, and just don't use
  boost::bind if you can.
* `size_t` is the correct container for all indexing of C++ structures (such
  as vectors). But keep in mind that the size of `size_t` is
  *platform-dependent*!
* Use size-specific types whenever interacting with hardware (`int32_t`, etc.).
  It's very easy to get bitten by incorrect sizes.
* Include include files in the following order: Local headers, other
  UHD headers, 3rd-party library headers, Boost headers, standard headers.
  The rationale is to include from most to least specific. This is the best way
  to catch missing includes (if you were to include the standard header first,
  it would be available to all include files that come later. If they need that
  standard header too, they should be including it themselves).
  Example:

```cpp
#include "x300_specific.hpp"
#include <uhd/utils/log.hpp>
#include <libusb/foo.hpp>
#include <boost/shared_ptr.hpp>
#include <mutex>
```

* Feel free to use modern C/C++ features even if they were not used before.
  Make sure they work with the compilers and dependencies which are set for the
  version of UHD the commit will be made upon. The Ettus CI system will be able
  to confirm this.
  For interesting new features, use 'anchor commits', which describe clearly
  which new feature was used. They can be used as a reference for other
  developers. Example:

```
commit 9fe731cc371efee7f0051186697e611571c5b41b
Author: Andrej Rode <andrej.rode@ettus.com>
Date:   Tue Nov 22 16:19:38 2016 -0800

    utils: uhd_find_devices display one device for each unique serial found
    
    Note: This also is the first precedent for the usage of the 'auto' keyword
    in UHD. Commits past this one will also be able to use 'auto'.
    
    Reviewed-By: Martin Braun <martin.braun@ettus.com>
```


## Boost-specific Guidelines

* Avoid Boost where possible.
* Don't use Boost's sleep functions. Prefer `std::chrono` functions.
* When using `boost::assign::list_of` or `boost::assign::map_list_of`, make
  sure to explicitly cast to the appropriate container (even better: Avoid
  `boost::assign`, maybe use std initializer lists where possible):

```cpp
std::vector<std::string> foo =
    boost::assign::list_of<std::string> // Explicitly list the type (in this
        ("string1")                     // case, std::string). Then list all
        ("string2")                     // the items (string1, string2, etc).
        ("etc")
    .to_container(foo); // Finally, call conversion routine explicitly.
// Same for maps:
std::map<std::string, std::string> bar =
    boost::assign::map_list_of<std::string, std::string>
        ("a", "b")
        ("c", "d")
        ("etc", "etc")
    .to_container(bar);
```


## Python-specific Guidelines

* Keep Python code compatible with Py2k and Py3k. There are plenty of tools to
  aid with this, such as `futurize`.
* Follow the suggestions in PEP8 (https://www.python.org/dev/peps/pep-0008/)
  and PEP257 (https://www.python.org/dev/peps/pep-0257/). The former is about
  code layout in general, the latter about docstrings.
* Pylint is good tool for helping with following code guidelines. It's very
  fussy though, so don't get too worked up about following its suggestions.


## Revision Control Hygiene

* In this repository, we almost always use fast-forward merges, and no merge
  commits.
* Prefix all commit message subject lines with the section of code they apply
  to, and use the imperative mood (Example: "x300: Fix overflow at full moon").
  Try and keep the subject line to 50 characters, but make 72 characters a hard
  limit.
* Follow up in greater detail in the body of the commmit message. The body is
  separated from the subject line with one blank line. Consider the body of the
  git commit an email to the future reader of this changeset, so don't be
  sparse in the commit body, and use it to explain the *what* and *why* of this
  commit (the "how" part should be obvious from the code change). Lines should
  be limited to 72 characters.
* Avoid refactoring, whitespace cleanup, or fixing code to match coding
  guidelines in the same commit as modifying behaviour. Prefer dedicated
  cleanup commits.
* Remember that we ship git repositories, not just code. This means every
  commit is part of our product and should be treated as such.

