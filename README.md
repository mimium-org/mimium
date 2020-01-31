# mimium

a programming language as an infrastructure for sound and music

[![Build Status](https://travis-ci.org/tomoyanonymous/mimium.svg?branch=master)](https://travis-ci.org/tomoyanonymous/mimium) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/de5190beb61f4ea9a337becdb21f8328)](https://www.codacy.com/manual/tomoyanonymous/mimium?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tomoyanonymous/mimium&amp;utm_campaign=Badge_Grade)

![mimium_logo_slanted](./mimium_logo_slant.svg)

mimium(*MInimal-Musical-medIUM*) is a domain specific programming language for describing/generating sound and music.

mimium is assumed to be used in a composition, rather than real-time performance.

It is much more similar to Musical Markup Languages though it have many programming-like features like loop and control sequence.

A final purpose is to edit a source code written in this language with gui as a programmable/parametric music creation software/DAW.

More specific info about language is currently in [design](design/design-proposal.md) section.

# Dependency

- llvm >= 9.0.0
- bison
- flex
- clang (depends on features of C++ 17)
- RtAudio
- RtMidi


# Author

Tomoya Matsuura 松浦知也

<https://matsuuratomoya.com/en>

# [License](LICENSE.md)

The source code is lisenced under [Mozilla Puclic License 2.0](https://www.mozilla.org/en-US/MPL/2.0/).


# Acknowledgements

This project is supported by 2019 Exploratory IT Human Resources Project ([The MITOU Program](https://www.ipa.go.jp/jinzai/mitou/portal_index.html)) by IPA: INFORMATION-TECHNOLOGY PROMOTION AGENCY, Japan.