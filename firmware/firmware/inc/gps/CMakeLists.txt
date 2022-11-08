if(CROSS_COMPILED)
# Build for testing on host.
target_sources(scbs_test PRIVATE
    nmea_utils.hh
    gps_utils.hh
)
else()
# Build for embedded target
target_sources(scbs PRIVATE
    nmea_utils.hh
    gps_utils.hh
    pa1616s.hh
)
endif()