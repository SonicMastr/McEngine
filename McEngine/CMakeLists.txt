cmake_minimum_required(VERSION 3.7)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain file")
set(VITASDK_CMAKE_FILE "$ENV{VITASDK}/share/vita.cmake" CACHE PATH "VitaSDK CMake functions file")
include("${VITASDK_CMAKE_FILE}" REQUIRED)

project(McOSU LANGUAGES C CXX)

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")

add_compile_definitions(__psp2__ __VITA__ MINIZ_NO_TIME)

add_compile_options(
  -O0 -g  # This O2 literally won't do anything for official builds btw
  -w
  -fno-common
)

include_directories(
    "src"
	  "src/App"
	  "src/Engine"
	  "src/GUI"
	  "src/Util"
    "src/App/FrameworkTest"
    #"src/App/Osu"
    "src/Engine/Input"
    "src/Engine/Main"
    "src/Engine/Platform"
    "src/Engine/Renderer"
    "src/GUI/Windows"
    "libraries/sdl_mixer/include"
    "$ENV{VITASDK}/arm-vita-eabi/include/SDL2"
    "$ENV{VITASDK}/arm-vita-eabi/include/freetype2"
    "$ENV{VITASDK}/arm-vita-eabi/include/freetype2/freetype"
)

# Build self

set(ELF "eboot.elf")
set(SELF "eboot.bin")

find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE simple_SRC
    # src/App/Osu/*.cpp
    src/App/FrameworkTest/*.cpp
    src/Engine/*.cpp
    src/GUI/*.cpp
    src/Util/*.cpp
    src/Util/*.c
)

add_executable("${ELF}"
    ${simple_SRC}
)

target_link_libraries("${ELF}"
  SDL2_mixer
  SDL2_image
  SDL2
  libgpu_es4_ext_stub_weak
  libIMGEGL_stub_weak
  libGLESv2_stub_weak
  SceAppMgr_stub
  SceProcessmgr_stub
  SceIme_stub_weak
  vorbisfile
  vorbis
  webp
  ogg
  mad
  mpg123
  FLAC
  modplug
  mikmod
  opusfile
  opus
  avformat
  avfilter
  swscale
  avutil
  avcodec
  swresample
  mp3lame
  freetype
  jpeg
  png
  SceMotion_stub
  SceCommonDialog_stub
  SceHid_stub
  SceGxm_stub
  SceAudio_stub
  SceAudioIn_stub
  ScePower_stub
  SceDisplay_stub
  SceCtrl_stub
  SceTouch_stub
  SceIofilemgr_stub
  SceNet_stub
  SceNetCtl_stub
  SceSsl_stub
  SceVshBridge_stub
  SceSysmodule_stub
  pthread
  ${SDL2_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  gcc
  zip
  z
  m
)

vita_create_self("${SELF}"
  "${ELF}"
  UNSAFE
  NOASLR
)