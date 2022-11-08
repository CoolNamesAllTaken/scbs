add_subdirectory(app)
add_subdirectory(gps)

if(CROSS_COMPILED)
# Build for testing on host.
target_include_directories(scbs_test PRIVATE
    gps
    app
)
else()
# Build for embedded target
target_include_directories(scbs PRIVATE
    gps
    app
)
endif()