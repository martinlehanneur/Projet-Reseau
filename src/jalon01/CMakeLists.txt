cmake_minimum_required(VERSION 2.8.3)

project(RE216_JALON01)
find_package (Threads)

#add here source files you need to compile and link to the client app
SET(SRC_CLIENT
	client.c
	)


add_executable(RE216_JALON01_CLIENT ${SRC_CLIENT})


#add here source files you need to compile and link to the server app
SET(SRC_SERVER
        server.c)


add_executable(RE216_JALON01_SERVER  ${SRC_SERVER})
target_link_libraries (RE216_JALON01_SERVER ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (RE216_JALON01_CLIENT ${CMAKE_THREAD_LIBS_INIT})
