# Maintainer: David J. Allen <davidallendj@gmail.com>
pkgname=gdpm
pkgver=0.0.1
pkgrel=1
pkgdesc="CLI tool to automate managing Godot game engine assets from the command-line. This includes a pre-built, static binary."
arch=('x86_64')
url="https://github.com/davidallendj/gdpm"
license=('MIT')
#groups=
depends=('glibc')
makedepends=('cmake', 'make', 'clang')
source=("https://github.com/davidallendj/gdpm/releases/download/v$pkgver/$pkgname.static.$arch.linux.static")
sha256sums=('012e3c32511d6d762ac070808e8bcae7f68dd261bf1cad3dbf4607c97aa1bb3d')

build () {
	# shouldn't need to build anything with static binary...
}

package() {
	cd "$srcdir/$pkgname-$pkgver"
	make DESTDIR="$pkgdir/" install
}