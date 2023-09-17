build_lib(
  LIBNAME csma
  SOURCE_FILES
    helper/csma-helper.cc
    model/backoff.cc
    model/csma-channel.cc
    model/csma-net-device.cc
    model/csma-net-device-anim.cc
  HEADER_FILES
    helper/csma-helper.h
    model/backoff.h
    model/csma-channel.h
    model/csma-net-device.h
    model/csma-net-device-anim.h
  LIBRARIES_TO_LINK ${libnetwork}
                    ${libnetanim}
  TEST_SOURCES ${test_sources}
)
