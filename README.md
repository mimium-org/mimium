# mimium

a programming language as an infrastructure for sound and music

stable: ![build status(master)](https://github.com/mimium-org/mimium/workflows/build%20&%20test/badge.svg?branch=master) dev: ![build status(dev)](https://github.com/mimium-org/mimium/workflows/build%20&%20test/badge.svg?branch=dev) 

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/de5190beb61f4ea9a337becdb21f8328)](https://www.codacy.com/manual/tomoyanonymous/mimium?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tomoyanonymous/mimium&amp;utm_campaign=Badge_Grade) [![License Badge-MPL2.0](https://img.shields.io/badge/LICENSE-MPLv2.0-blue)](./LICENSE.md)

![mimium_logo_slanted](./mimium_logo_slant.svg)

mimium(*MInimal-Musical-medIUM*) is a domain specific programming language for describing/generating sound and music.

With this language, you can write a low-level audio processing with an easy expression and high-performance powered by LLVM.

```rust
fn lpf(input:float,fb:float){    
    return (1-fb)*input + fb*self
}
```

A special keyword `self` can be used in function, which is a last return value of the function.
This enables an easy and clean expression of feedback connection of signal chain, inspired by [Faust](https://faust.grame.fr).

you can also write a note-level processing by using a temporal recursion, inspired by [Extempore](https://extemporelang.github.io/).

```rust

fn noteloop()->void{
    freq =  (freq+1200)%4000
    noteloop()@(now + 48000)
}

```

Calling function with `@` specifies the time when the function will be executed.
An event scheduling for this mechanism is driven by a clock from an audio driver thus have a sample-accuracy.

<!-- More specific info about language is currently in [design](design/design-proposal.md) section. -->
# Download

You can download from [release](https://github.com/mimium-org/mimium/releases) section.
Currently only macOS is supported. Windows, Linux and web browser will be supported for future.

# Build from Source

(not recommended yet. also see travis.yml)

## dependency

- cmake
- llvm >= 9.0.0(RTTI need to be activated)
- bison >= 3.3
- flex
- Libsndfile

- RtAudio(cmake will automatically download)

```sh
mkdir build && cd build
# configure. if you want to install to specific directory, add -DCMAKE_INSTALL_PREFIX=/your/directory
cmake .. 
# build
cmake --build . --target default_build -j
#install
cmake --build . --target install
```

### notes for linux build

Currently tested building on Ubuntu 18.04(Bionic) and 20.04(Focal)
For a compiler, GCC>9 is recommended.

At Bionic, a version of bison from apt is 3.0.4, which will not work. Please install latest version manually.(See also github action page.)

# Author

Tomoya Matsuura 松浦知也

<https://matsuuratomoya.com/en>

# [License](LICENSE.md)

The source code is lisenced under [Mozilla Puclic License 2.0](LICENSE.md).

The source code uses some third party libraries with BSD-like lincenses, see [COPYRIGHT](./COPYRIGHT).

# Acknowledgements

This project is supported by 2019 Exploratory IT Human Resources Project ([The MITOU Program](https://www.ipa.go.jp/jinzai/mitou/portal_index.html)) by IPA: INFORMATION-TECHNOLOGY PROMOTION AGENCY, Japan.