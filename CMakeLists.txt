cmake_minimum_required(VERSION 2.8)

project(ZAC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wreturn-type")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(INCLUDE_PATH
		chipmunk
		win32/include
		win32/include/GL
		win32/include/AL
		win32/include/IL
		win32/include/SDL
	)

	set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib)

	set(SDL_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/SDL.lib)
	set(GL_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/libopengl32.a)
	set(GLEW_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/libglew32.dll.a)
	set(chipmunk_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/libchipmunk.dll.a)
	set(IL_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/DevIL.lib)
	set(ILU_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/ILU.lib)
	set(ILUT_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/ILUT.lib)
	set(AL_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/OpenAL32.lib)
	set(alut_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/alut.lib)

	set(LIBS
		${GLEW_LIBS}
		${GL_LIBS}
		${SDL_LIBS}
		${chipmunk_LIBS}
		${IL_LIBS}
		${ILU_LIBS}
		${ILUT_LIBS}
		${AL_LIBS}
		${alut_LIBS}
	)
else()
	set(INCLUDE_PATH
		/usr/include/chipmunk
		/usr/include
		/usr/include/SDL
		/usr/include/IL
	)
	set(LIBS 
		SDL
		GL
		GLEW
		IL
		ILU
		ILUT
		openal
		alut
		chipmunk
	)
endif()

include_directories(
	.
	${INCLUDE_PATH}
)

add_executable(zac
	gtdc.cpp
	gui.cpp
	human.cpp
	world.cpp
	renderer.cpp
)
target_link_libraries(zac ${LIBS})



