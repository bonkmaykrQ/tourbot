add_executable(pengobot
    main.cpp
)
target_link_libraries(pengobot curl -static-libgcc -static-libstdc++)
