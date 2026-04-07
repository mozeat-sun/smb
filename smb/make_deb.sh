#!/bin/bash

set -euo pipefail

# === Configuration ===
PKG_NAME="zoo-smb"
PKG_VERSION="${PKG_VERSION:-1.0.0}"
PKG_ARCH="amd64"
PKG_MAINTAINER="Your Name <your@email.com>"
PKG_DESCRIPTION="Zoo SMB Transport Library and tools"
PKG_SECTION="utils"
PKG_PRIORITY="optional"
PKG_DEPENDS="libc6 (>= 2.27)"
BUILD_DIR="${BUILD_DIR:-build}"
DEB_DIR="deb_build"
INSTALL_PREFIX="/usr/local"

# === Prepare directories ===
rm -rf "$DEB_DIR"
mkdir -p "$DEB_DIR/DEBIAN"
mkdir -p "$DEB_DIR${INSTALL_PREFIX}/bin"
mkdir -p "$DEB_DIR${INSTALL_PREFIX}/lib"
mkdir -p "$DEB_DIR${INSTALL_PREFIX}/include"
mkdir -p "$DEB_DIR${INSTALL_PREFIX}/share/doc/$PKG_NAME"

# === Copy files ===
# Binaries
if [ -d "$BUILD_DIR/bin" ]; then
    cp -a "$BUILD_DIR/bin/." "$DEB_DIR${INSTALL_PREFIX}/bin/"
elif [ -d "$BUILD_DIR" ]; then
    # fallback: copy all executables in build dir
    find "$BUILD_DIR" -maxdepth 1 -type f -executable -exec cp {} "$DEB_DIR${INSTALL_PREFIX}/bin/" \;
fi

# Libraries
if [ -d "$BUILD_DIR/lib" ]; then
    cp -a "$BUILD_DIR/lib/." "$DEB_DIR${INSTALL_PREFIX}/lib/"
fi

# Headers
if [ -d "include" ]; then
    cp -a "include/." "$DEB_DIR${INSTALL_PREFIX}/include/"
fi

# Documentation
if [ -f "README.md" ]; then
    cp "README.md" "$DEB_DIR${INSTALL_PREFIX}/share/doc/$PKG_NAME/"
fi
if [ -f "LICENSE" ]; then
    cp "LICENSE" "$DEB_DIR${INSTALL_PREFIX}/share/doc/$PKG_NAME/"
fi

# === Create control file ===
cat > "$DEB_DIR/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $PKG_VERSION
Section: $PKG_SECTION
Priority: $PKG_PRIORITY
Architecture: $PKG_ARCH
Maintainer: $PKG_MAINTAINER
Depends: $PKG_DEPENDS
Description: $PKG_DESCRIPTION
EOF

# === (Optional) Postinst script example ===
# cat > "$DEB_DIR/DEBIAN/postinst" <<EOF
# #!/bin/bash
# ldconfig
# EOF
# chmod 755 "$DEB_DIR/DEBIAN/postinst"

# === Set permissions ===
find "$DEB_DIR" -type d -exec chmod 755 {} \;
find "$DEB_DIR" -type f -exec chmod 644 {} \;
find "$DEB_DIR${INSTALL_PREFIX}/bin" -type f -exec chmod 755 {} \; 2>/dev/null || true
find "$DEB_DIR${INSTALL_PREFIX}/lib" -type f -exec chmod 755 {} \; 2>/dev/null || true

# === Build the .deb package ===
DEB_PKG="${PKG_NAME}_${PKG_VERSION}_${PKG_ARCH}.deb"
dpkg-deb --build "$DEB_DIR" "$DEB_PKG"

echo "Debian package created: $DEB_PKG"