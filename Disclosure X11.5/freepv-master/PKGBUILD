# This is an example PKGBUILD file. Use this as a start to creating your own,
# and remove these comments. For more information, see 'man PKGBUILD'.
# NOTE: Please fill out the license field for your package! If it is unknown,
# then please put 'unknown'.

# Maintainer: Your Name <youremail@domain.com>
pkgname=freepv
pkgver=0.3.0-r6
pkgdesc="OpenGL accelerated panorama viewer and browser plugin (supports Quicktime, PangeaVR and GLPanoView panoramas)"
arch=('x86_64')
url="https://sourceforge.net/projects/freepv/"
license=('LGPL-2.1')
groups=()
depends=('libxml2' 'libpng' 'freeglut' 'zlib' 'libjpeg-turbo' 'libxmu' 'libxt' 'libxxf86vm' 'mesa' 'glu')
makedepends=('cmake')
source=("freepv::git+https://github.com/bartoszwalicki/freepv.git")


build() {
	cd "freepv"
	cmake -DCMAKE_INSTALL_PREFIX=/usr/local .
	make
}

package() {
	cd "freepv"
	make install
}
md5sums=('SKIP')