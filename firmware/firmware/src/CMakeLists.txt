add_subdirectory(gps)
add_subdirectory(app)

if(CROSS_COMPILED)
# Build for testing on host.
target_include_directories(scbs_test PRIVATE
    display
    gps
    app
)
# Don't include main for testing.
else()
# Build for embedded target
target_include_directories(scbs PRIVATE
    display
    gps
    app
)
target_sources(scbs
PRIVATE
    main.cpp
)
endif()