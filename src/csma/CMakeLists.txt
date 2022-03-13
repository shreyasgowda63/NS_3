build_lib(
  LIBNAME csma
  SOURCE_FILES
    helper/csma-helper.cc
    model/backoff.cc
    model/csma-channel.cc
    model/csma-net-device.cc
    model/csma-net-device-state.cc
  HEADER_FILES
    helper/csma-helper.h
    model/backoff.h
    model/csma-channel.h
    model/csma-net-device.h
    model/csma-net-device-state.h
  LIBRARIES_TO_LINK ${libnetwork}
  TEST_SOURCES
    test/csma-test.cc
    test/csma-net-device-state-test.cc
)
