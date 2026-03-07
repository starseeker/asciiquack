This repository is intended to become a C++17 implementation of asciidoctor.

It has been initialized with the upstream implementation in its native language -
the overall goal is to replace that implementation with a self contained,
stand-alone C++17 program using only the standard libraries that can deal with
asciidoc conversion for us.

The initial stage is to put together a plan - design what the code will look
like, map out the various stages of translation, and establishing a testing
setup that will be able to verify the C++ version is working correctly.

The desired end state is a clean, self-contained, strictly C++17 compliant codebase
with no compiler warnings that is well documented, security hardened, and able
to properly handle most real-world asciidoc inputs.  If there are particular
obscure features that would require heavy dependencies or be extremely complex
to handle you can itemize them for further consideration, but the hope is we
will be able to "drop in" asciiquack to an asciidoc based documentation workflow
and have it work.  Command line option compatibility would be nice but is not
required.  You may use https://github.com/jarro2783/cxxopts for option handling -
the header is included in the repository already.
