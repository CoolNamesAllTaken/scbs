if(CROSS_COMPILED)
# Build for testing on host.
target_sources(scbs_test PRIVATE
    nmea_utils.cc
    gps_utils.cc
)
else()
# Build for embedded target
target_sources(scbs PRIVATE
    nmea_utils.cc
    gps_utils.cc
    pa1616s.cc
)
endif()