# mimium

a programming language as an infrastructure for sound and music

stable: [![build status(master)](https://github.com/mimium-org/mimium/workflows/build%20&%20test/badge.svg?branch=master)](https://github.com/mimium-org/mimium/actions) dev: [![build status(dev)](https://github.com/mimium-org/mimium/workflows/build%20&%20test/badge.svg?branch=dev)](https://github.com/mimium-org/mimium/actions) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/a7171f079d2b4439971513b6358c5a35)](https://www.codacy.com/gh/mimium-org/mimium/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=mimium-org/mimium&amp;utm_campaign=Badge_Grade)

[![website badge](https://img.shields.io/badge/mimium.org-Website-d6eff7)](https://mimium.org) [![Gitter](https://badges.gitter.im/mimium-dev/community.svg)](https://gitter.im/mimium-dev/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) [![License Badge-MPL2.0](https://img.shields.io/badge/LICENSE-MPLv2.0-blue)](./LICENSE.md)

![mimium_logo_slanted](./mimium_logo_slant.svg)

mimium(*MInimal-Musical-medIUM*) is a programming language for sound and music.

mimium is made to be an infrastructure for distributing music in a form of a source code, not only a tool for musicians and programmers.

Its syntax and semantics are technically inspired from several modern programming languages for sound such as *[Faust](https://faust.grame.fr)* and *[Extempore](https://extemporelang.github.io/)*.

You can write various expression from low-level signal processing to note-level processing with a simple syntax and high-performance.
Since all the code is executed with JIT compilation using LLVM which has an equivalent performance as the dsp code written in general purpose languages such as C++.

A minimal example below generates a sinewave of 440Hz.

```rust
// minimal.mmm
twopi = 3.141595*2
sr = 48000
fn dsp(){
    out = sin(now * 440 * twopi / sr)
    return (out,out)
}
```

To run the code, type `mimium path/minimal.mmm` on your terminal application.

A special keyword `self` can be used in function, which is a last return value of the function.
This enables an easy and clean expression of feedback connection of signal chain.

```rust
fn lpf(input:float,fb:float){    
    return (1-fb)*input + fb*self
}
```

You can also write a note-level processing by using `@` operator which specifies the time when the function will be executed. Another special keyword `now` can be used for getting current logical time.
An event scheduling is sample-accurate because the scheduler is driven by an audio driver.

```rust
freq = 440
fn noteloop()->void{
    freq = (freq+1200)%4000
    noteloop()@(now + 48000)
}
```

More specific infos about the language are on [mimium Website](https://mimium.org).

## Installation

mimium can be run on macOS(x86), Linux(ALSA backend), Windows(WASAPI backend). WebAssemby backend will be supported for future.

An easiest way to getting started is to use [Visual Studio Code extension](https://marketplace.visualstudio.com/items?itemName=mimium-org.mimium-language). Search "mimium" in extension tab and install it. When you create & open the file with the file extension `.mmm`, you will be asked to install the latest binary. The extension also contains syntax highlights for `.mmm` files.

On macOS and Linux, installation via [Homebrew](https://brew.sh/) is recommended.

You can install mimium with a command as follows.

```sh
brew install mimium-org/mimium/mimium
```

Also, you can get a built binary from [release](https://github.com/mimium-org/mimium/releases) section.
## Build from Source

To build on Windows, you need to use MSYS2. For details, check [GitHub Action Workflow](https://github.com/mimium-org/mimium/blob/dev/.github/workflows/build_and_test.yml) and documentations on official website ([Installation](https://mimium.org/en/docs/users-guide/getting-started/installation/) and [Setting up development environment](https://mimium.org/en/docs/developers-guide/setup-development-environments/)).
### Installing Dependencies

- cmake
- bison >= 3.3
- flex
- llvm >= 11
- Libsndfile
- RtAudio(cmake will automatically download)

#### macOS

Install [homebrew](https://brew.sh) and XCode Commandline Tools beforehand.

```sh
brew install cmake flex bison libsndfile llvm ninja
```

#### Linux(Ubuntu)

*On Linux(Ubuntu), we recommend to install llvm using an automatic installation script in https://apt.llvm.org/ because `llvm` package in apt does not contain some libs required by `llvm-config --libs`*

```sh
pushd /tmp && wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && sudo ./llvm.sh && popd
sudo apt-get install libalsa-ocaml-dev libfl-dev libbison-dev libz-dev libvorbis-dev libsndfile-dev libopus-dev gcc-9 ninja-build
```

#### Windows(MSYS2,mingw64)

Install [msys2](https://www.msys2.org/) by following instruction on the website. Launch Mingw64 terminal.

```sh
pacman -Syu git flex bison mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw64/mingw-w64-x86_64-libsndfile mingw64/mingw-w64-x86_64-opus mingw-w64-x86_64-ninja mingw-w64-x86_64-llvm
```

### Clone Repository, build and install

```sh
git clone https://github.com/mimium-org/mimium
cd mimium
# configure. if you want to install to specific directory, add -DCMAKE_INSTALL_PREFIX=/your/directory
cmake -Bbuild
# build
cmake --build build -j
# install
cmake --build build --target install
```
## Author

Tomoya Matsuura/松浦知也

<https://matsuuratomoya.com/en>

## [License](LICENSE.md)

The source code is lisenced under [Mozilla Puclic License 2.0](LICENSE.md).

The source code contains third party libraries with BSD-like lincenses, see [COPYRIGHT](./COPYRIGHT).

## Acknowledgements

This project is supported by all the contributers, [Sponsors](https://github.com/sponsors/tomoyanonymous), grants and scholarships as follows.

- 2019 Exploratory IT Human Resources Project ([The MITOU Program](https://www.ipa.go.jp/jinzai/mitou/portal_index.html)) by IPA: INFORMATION-TECHNOLOGY PROMOTION AGENCY, Japan.
- Kakehashi Foundation

### Contributors

<!-- ALL-CONTRIBUTORS-BADGE:START - Do not remove or modify this section -->
[![All Contributors](https://img.shields.io/badge/all_contributors-8-orange.svg?style=flat-square)](#contributors-)
<!-- ALL-CONTRIBUTORS-BADGE:END --> 

	
<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tr>
    <td align="center"><a href="https://t-sin.github.io"><img src="https://avatars.githubusercontent.com/u/4403863?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Shinichi Tanaka</b></sub></a><br /><a href="https://github.com/mimium-org/mimium/commits?author=t-sin" title="Documentation">📖</a> <a href="https://github.com/mimium-org/mimium/commits?author=t-sin" title="Code">💻</a></td>
    <td align="center"><a href="http://deepdrilledwell.secret.jp/ddw/"><img src="https://avatars.githubusercontent.com/u/74606612?v=4?s=100" width="100px;" alt=""/><br /><sub><b>kyo</b></sub></a><br /><a href="https://github.com/mimium-org/mimium/commits?author=syougikakugenn" title="Documentation">📖</a></td>
    <td align="center"><a href="http://baku89.com"><img src="https://avatars.githubusercontent.com/u/2124392?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Baku 麦</b></sub></a><br /><a href="#financial-baku89" title="Financial">💵</a></td>
    <td align="center"><a href="https://github.com/yuichkun"><img src="https://avatars.githubusercontent.com/u/14039540?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Yuichi Yogo</b></sub></a><br /><a href="#financial-yuichkun" title="Financial">💵</a></td>
    <td align="center"><a href="http://ayumu-nagamatsu.com"><img src="https://avatars.githubusercontent.com/u/7838131?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Ayumu Nagamatsu</b></sub></a><br /><a href="#financial-nama-gatsuo" title="Financial">💵</a></td>
    <td align="center"><a href="https://horol.org"><img src="https://avatars.githubusercontent.com/u/3610296?v=4?s=100" width="100px;" alt=""/><br /><sub><b>zigen</b></sub></a><br /><a href="#financial-zigen" title="Financial">💵</a></td>
    <td align="center"><a href="http://hitoshitakeuchi.com"><img src="https://avatars.githubusercontent.com/u/6305267?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Hitoshi Takeuchi</b></sub></a><br /><a href="#financial-hitoshitakeuchi" title="Financial">💵</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/Inqb8tr-jp"><img src="https://avatars.githubusercontent.com/u/79005925?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Inqb8tr-jp</b></sub></a><br /><a href="#financial-Inqb8tr-jp" title="Financial">💵</a> <a href="#infra-Inqb8tr-jp" title="Infrastructure (Hosting, Build-Tools, etc)">🚇</a></td>
  </tr>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->