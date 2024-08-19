echo "Build protobuf-3.6.1.3..."
ROOT_PATH=$(pwd)
INSTALL_PREFIX=../lib/x64

cmake -S protobuf-3.6.1.3/cmake -B protobuf-3.6.1.3/Debug -DCMAKE_POSITION_INDEPENDENT_CODE=ON -Dprotobuf_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Debug # -DCMAKE_INSTALL_PREFIX=${INSTALL_PATH}/protobuf
cmake -S protobuf-3.6.1.3/cmake -B protobuf-3.6.1.3/Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -Dprotobuf_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release # -DCMAKE_INSTALL_PREFIX=${INSTALL_PATH}/protobuf
cmake --build ./protobuf-3.6.1.3/Debug  --target install -j ${CPU_CORE_NUM}
cmake --build ./protobuf-3.6.1.3/Release --target install -j ${CPU_CORE_NUM}

mv -f ./protobuf-3.6.1.3/Debug/*.a ${INSTALL_PREFIX}
mv -f ./protobuf-3.6.1.3/Release/*.a ${INSTALL_PREFIX}
cd ${ROOT_PATH}