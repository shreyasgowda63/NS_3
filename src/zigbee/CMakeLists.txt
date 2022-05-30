build_lib(
  LIBNAME zigbee
  SOURCE_FILES
    helper/zigbee-helper.cc
    model/zigbee-nwk-fields.cc
    model/zigbee-nwk.cc
    model/zigbee-stack.cc
    model/zigbee-stack-container.cc
    model/zigbee-nwk-header.cc
    model/zigbee-payload-header.cc
    model/zigbee-tables.cc
  HEADER_FILES
    helper/zigbee-helper.h
    model/zigbee-nwk-fields.h
    model/zigbee-nwk.h
    model/zigbee-stack.h
    model/zigbee-stack-container.h
    model/zigbee-nwk-header.h
    model/zigbee-payload-header.h
    model/zigbee-tables.h
  LIBRARIES_TO_LINK ${liblr-wpan}
  TEST_SOURCES
)
