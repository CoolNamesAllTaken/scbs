if(CROSS_COMPILED)
# Build for testing on host.
target_sources(scbs_test PRIVATE

)
else()
# Build for embedded target
target_sources(scbs PRIVATE

)
endif()