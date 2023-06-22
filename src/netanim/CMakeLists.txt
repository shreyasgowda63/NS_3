build_lib(
  LIBNAME netanim
  SOURCE_FILES model/animation-interface.cc
               model/net-device-anim.cc
  HEADER_FILES model/animation-interface.h
               model/net-device-anim.h
  LIBRARIES_TO_LINK
    ${libinternet}
    ${libmobility}
    ${libwimax}
    ${libwifi}
    ${libuan}
    ${libenergy}
    ${liblr-wpan}
    ${libwave}
    ${libpoint-to-point-layout}
  TEST_SOURCES test/netanim-test.cc
)
