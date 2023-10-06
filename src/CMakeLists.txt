add_executable(pengobot
    main.cpp
)
target_link_libraries(pengobot ${CURL_LIBRARIES} -static)
