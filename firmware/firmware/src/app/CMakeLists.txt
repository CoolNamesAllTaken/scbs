if(CROSS_COMPILED)
# Build for testing on host.
target_sources(scbs_test PRIVATE
    scbs_comms.cc
    scbs.cc
)
else()
# Build for embedded target
target_sources(scbs PRIVATE
    scbs_comms.cc
    scbs.cc
)
endif()