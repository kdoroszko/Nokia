set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)

set(SOURCES ${SOURCES})

add_executable(client_run chat_client.cpp chat_message.hpp)
target_link_libraries(client_run pthread stdc++fs)

add_executable(server_run chat_server.cpp chat_message.hpp)
target_link_libraries(server_run pthread stdc++fs)

add_library(${BINARY}_lib STATIC ${SOURCES})

if(Boost_FOUND)
    target_link_libraries (client_run ${Boost_LIBRARIES})
    message("-- Boost link to: ${Boost_VERSION_STRING}")
else()
    message(FATAL_ERROR "Boost not found!")
endif()

if(Boost_FOUND)
    target_link_libraries (server_run ${Boost_LIBRARIES})
    message("-- Boost link to: ${Boost_VERSION_STRING}")
else()
    message(FATAL_ERROR "Boost not found!")
endif()
