cmake_minimum_required(VERSION 2.8)
project(liepaTTS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

list(APPEND CMAKE_MODULE_PATH "$ENV{HOME}/myWorktree/ctc-linux64-atom-2.8.6.23-20191127_161825/yocto-sdk/share/cmake")
link_directories($ENV{HOME}/myWorktree/ctc-linux64-atom-2.8.6.23-20191127_161825/yocto-sdk/sysroots/core2-32-sbr-linux/usr/lib)

SET(ALCOMMON_DIR "$ENV{HOME}/myWorktree/naoqi-sdk-2.8.5.10-linux64/share/cmake/alcommon/")
SET(BOOST_SIGNALS_INCLUDE_DIRS "$ENV{HOME}/myWorktree/naoqi-sdk-2.8.5.10-linux64/include/boost")
SET(QI_DIR "$ENV{HOME}/myWorktree/ctc-linux64-atom-2.8.6.23-20191127_161825/qi-framework/share/cmake/qi/")

find_package(qibuild)
find_package(ALCOMMON)
find_package(BOOST)

set(_srcs_sint
    src/lib/TextNormalization.cpp
    src/lib/TextNormalization.h
    src/lib/stringWithLetterPosition.cpp
    src/lib/stringWithLetterPosition.h
    src/lib/transcrLUSS.cpp
    src/lib/transcrLUSS.h
    src/lib/rezultC.h
    src/lib/Transkr.cpp
    src/lib/Transkr.h
    src/lib/Skiemen.cpp
    src/lib/Skiemen.h
    src/lib/Kircdb.cpp
    src/lib/Kircdb.h
    src/lib/nb12.h
    src/lib/db8r1_14.h
    src/lib/Vb9r1_13.h
    src/lib/vgal.h
    src/lib/ArKirciuoti.cpp
    src/lib/ArKirciuoti.h
    src/lib/LithUSS.cpp
    src/lib/LithUSS_Error.h
    src/lib/UnitSelection.cpp
    src/lib/UnitSel.h
    src/lib/fv2id.cpp
    src/lib/fv2id.h
    src/lib/Euristika.cpp
    src/lib/greitis.cpp
    src/lib/RateChange.cpp
    src/lib/RateChange.h
    src/lib/RateChange1.h
    src/lib/VeiksmaiSuFailais.cpp
    src/lib/ilgiai.cpp
    src/lib/ilgiai.h
    src/lib/strtokf.cpp)

qi_create_lib(transcr_sint_LUSS STATIC ${_srcs_sint})

set(_srcs
    src/pijausmodule.cpp
    src/pijausmodule.h
    src/LithUSS.h
    src/main.cpp)

qi_create_bin(LiepaTTS SRC ${_srcs} DEPENDS BOOST QI)
target_link_libraries(LiepaTTS transcr_sint_LUSS)
target_link_libraries(LiepaTTS -lasound)
