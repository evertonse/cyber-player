pacman --noconfirm -S --needed --overwrite * \
    mingw-w64-x86_64-cc \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-perl \
    mingw-w64-x86_64-pkg-config \
    mingw-w64-x86_64-clang-tools-extra

cmake -S . -B build -GNinja \
    -Wdeprecated -Wdev -Werror \
     \
    -DSDL_WERROR=true \
    -DSDL_EXAMPLES=true \
    -DSDL_TESTS=true \
    -DSDLTEST_TRACKMEM=ON \
    -DSDL_INSTALL_TESTS=true \
    -DSDL_CLANG_TIDY=true \
    -DSDL_DISABLE_INSTALL_DOCS=OFF \
    -DSDL_DISABLE_INSTALL_CPACK=OFF \
    -DSDL_DISABLE_INSTALL_DOCS=OFF \
    -DSDLTEST_PROCDUMP=ON \
    -DSDL_SHARED=true \
    -DSDL_STATIC=true \
    -DSDL_VENDOR_INFO="Github Workflow" \
    -DCMAKE_INSTALL_PREFIX=prefix \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
