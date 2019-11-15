# Download Xerces-C++ 3.2.2 from: http://xerces.apache.org/xerces-c/download.cgi
# Extract to ./libs/

cd libs/xerces-c-3.2.2
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=../bin ..
cmake --build . --config Release --target install