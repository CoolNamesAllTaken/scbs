add_subdirectory(app)

if(CROSS_COMPILED)
# Build for testing on host.
target_include_directories(scbs_test PRIVATE
    app
)
else()
# Build for embedded target
target_include_directories(scbs PRIVATE
    app
)
endif()