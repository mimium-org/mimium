# Contributer: Tomoya Matsuura <me at matsuuratomoya dot com>
pkgname=mimium
pkgver=0.3.0
pkgrel=0
pkgdesc='An infrastructural programming language for sound and music'
arch=('x86_64')
url="https://mimium.org"
license=('MPL 2.0')
depends=('libasound.so' 'libsndfile.so')
makedepends=('cmake' 'bison' 'flex' 'llvm' 'libsndfile' 'rtaudio')
source=("https://github.com/mimium-org/mimium/archive/v$pkgver.zip")
sha256sums=('c3de91760cc0985bb24d5564ec81a7f0f8744045c1eee81fc9ba8d99fe2e5425')
build() {
  cmake -B build -S "${pkgname}-${pkgver}" \
  		-DCMAKE_INSTALL_PREFIX='/usr'
  cmake --build build -j
}

package() {
  cd "build"
  make DESTDIR="$pkgdir/" install 
}

