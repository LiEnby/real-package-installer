cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(real_package_installer)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "★RealPackage Installer")
set(VITA_TITLEID  "RPKG10031")

set(VITA_VERSION  "01.01")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS}")
set(VITA_MAKE_FSELF_FLAGS "${VITA_MAKE_FSELF_FLAGS} -a 0x280000000000002E")


add_subdirectory(kern)

include_directories(
	kern
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/kern
)

add_executable(${PROJECT_NAME}
	app/ctrl.c
	app/draw.c
	app/kernel.c
	app/io.c
	app/main.c
	app/menu.c
	app/pkg.c
	app/sfo.c
	app/psp.c
	app/sha256.c
	app/promote.c
	app/rif.c
	app/ime.c
	app/bgm.cpp
)

add_dependencies(real_package_installer kplugin-stubs)
add_dependencies(real_package_installer kplugin.skprx-self)

target_link_libraries(${PROJECT_NAME}
	vita2d
	soloud
	pthread
	stdc++
	freetype
	png
	jpeg
	m
	z

	real_pkg_installer_helper_stub
	taihen_stub

	SceNpDrm_stub
	SceDisplay_stub
	SceGxm_stub
	ScePgf_stub
	ScePvf_stub
	SceCommonDialog_stub
	SceCtrl_stub
	SceVshBridge_stub
	SceAppMgr_stub
	SceAppUtil_stub
	SceIofilemgr_stub
	ScePower_stub
	SceShellSvc_stub
	SceAudio_stub
	ScePromoterUtil_stub
	SceSysmodule_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)


vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}

  FILE ${CMAKE_CURRENT_BINARY_DIR}/kern/kplugin.skprx kplugin.skprx
  
  FILE app/res/sce_sys/icon0.png sce_sys/icon0.png
  FILE app/res/sce_sys/pic0.png sce_sys/pic0.png
  FILE app/res/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE app/res/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  
  FILE app/res/menuoverlay.png res/menuoverlay.png
  FILE app/res/background.png res/background.png
  FILE app/res/bgm.ogg res/bgm.ogg  
)
