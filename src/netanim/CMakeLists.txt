build_lib(
  LIBNAME netanim
  SOURCE_FILES model/animation-interface.cc
  HEADER_FILES model/animation-interface.h
  LIBRARIES_TO_LINK
    ${libinternet}
    ${libmobility}
    ${libwimax}
    ${libuan}
    ${libenergy}
    ${liblr-wpan}
)
