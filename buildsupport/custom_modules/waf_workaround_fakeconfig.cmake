# cmake-format: off
#
# A sample of what Waf currently produces

# ---- Summary of optional NS-3 features:
# Build profile                 : debug
# Build directory               :
# BRITE Integration             : not enabled (BRITE not enabled (see option --with-brite))
# DES Metrics event collection  : not enabled (defaults to disabled)
# DPDK NetDevice                : not enabled (libdpdk not found, $RTE_SDK and/or $RTE_TARGET environment variable not set or incorrect)
# Emulation FdNetDevice         : enabled
# Examples                      : not enabled (defaults to disabled)
# File descriptor NetDevice     : enabled
# GNU Scientific Library (GSL)  : enabled
# Gcrypt library                : not enabled (libgcrypt not found: you can use libgcrypt-config to find its location.)
# GtkConfigStore                : enabled
# MPI Support                   : not enabled (option --enable-mpi not selected)
# NS-3 Click Integration        : not enabled (nsclick not enabled (see option --with-nsclick))
# NS-3 OpenFlow Integration     : not enabled (OpenFlow not enabled (see option --with-openflow))
# Netmap emulation FdNetDevice  : not enabled (needs net/netmap_user.h)
# Network Simulation Cradle     : not enabled (NSC not found (see option --with-nsc))
# PlanetLab FdNetDevice         : not enabled (PlanetLab operating system not detected (see option --force-planetlab))
# PyViz visualizer              : not enabled (Missing python modules: pygraphviz, gi.repository.GooCanvas)
# Python API Scanning Support   : not enabled (castxml too old)
# Python Bindings               : enabled
# Real Time Simulator           : enabled
# SQLite stats support          : enabled
# Tap Bridge                    : enabled
# Tap FdNetDevice               : enabled
# Tests                         : not enabled (defaults to disabled)
# Threading Primitives          : enabled
# Use sudo to set suid bit      : not enabled (option --enable-sudo not selected)
# XmlIo                         : enabled
#
#
# And now a sample after build
#
# Modules built:
# antenna                   aodv                      applications
# bridge                    buildings                 config-store
# core                      csma                      csma-layout
# dsdv                      dsr                       energy
# fd-net-device             flow-monitor              internet
# internet-apps             lr-wpan                   lte
# mesh                      mobility                  netanim
# network                   nix-vector-routing        olsr
# point-to-point            point-to-point-layout     propagation
# sixlowpan                 spectrum                  stats
# tap-bridge                test (no Python)          topology-read
# traffic-control           uan                       virtual-net-device
# wave                      wifi                      wimax
#
# Modules not built (see ns-3 tutorial for explanation):
# brite                     click                     dpdk-net-device
# mpi                       openflow                  visualizer
#
# cmake-format: on

# Now the CMake part

macro(check_on_or_off user_config_switch confirmation_flag)
  if(${user_config_switch})
    if(${confirmation_flag})
      string(APPEND out "ON\n")
    else()
      string(APPEND out "OFF (not found)\n")
    endif()
  else()
    string(APPEND out "OFF\n")
  endif()
endmacro()

function(print_formatted_table_with_modules table_name modules output)
  set(temp)
  string(APPEND temp "${table_name}:\n")
  set(count 0) # Variable to count number of columns
  set(width 26) # Variable with column width
  string(REPLACE "lib" "" modules_to_print "${modules}")
  list(SORT modules_to_print) # Sort for nice output
  foreach(module ${modules_to_print})
    # Get the size of the module string name
    string(LENGTH ${module} module_name_length)

    # Calculate trailing spaces to fill the column
    math(EXPR num_trailing_spaces "${width} - ${module_name_length}")

    # Get a string with spaces
    string(RANDOM LENGTH ${num_trailing_spaces} ALPHABET " " trailing_spaces)

    # Append module name and spaces to output
    string(APPEND temp "${module}${trailing_spaces}")
    math(EXPR count "${count} + 1") # Count number of column

    # When counter hits the 3rd column, wrap to the nextline
    if(${count} EQUAL 3)
      string(APPEND temp "\n")
      set(count 0)
    endif()
  endforeach()
  string(APPEND temp "\n")
  # Save the table outer scope out variable
  set(${output} ${${output}}${temp} PARENT_SCOPE)
endfunction()

macro(write_fakewaf_config)
  set(out "---- Summary of optional NS-3 features:\n")
  string(APPEND out "Build profile                 : ${build_profile}\n")
  string(APPEND out
         "Build directory               : ${CMAKE_OUTPUT_DIRECTORY}\n"
  )
  string(APPEND out "BRITE Integration             : ")
  check_on_or_off("ON" "${NS3_BRITE}")

  string(APPEND out "DES Metrics event collection  : ${NS3_DES_METRICS}\n")
  string(APPEND out "DPDK NetDevice                : ")
  check_on_or_off("ON" "${ENABLE_DPDKDEVNET}")

  string(APPEND out "Emulation FdNetDevice         : ")
  check_on_or_off("${NS3_EMU}" "${ENABLE_EMU}")

  string(APPEND out "Examples                      : ${EXAMPLES_ENABLED}\n")
  string(APPEND out "File descriptor NetDevice     : ")
  check_on_or_off("ON" "${ENABLE_FDNETDEV}")

  string(APPEND out "GNU Scientific Library (GSL)  : ")
  check_on_or_off("${NS3_GSL}" "${GSL_FOUND}")

  # string(APPEND out "Gcrypt library                : not enabled (libgcrypt
  # not found: you can use libgcrypt-config to find its location.)

  string(APPEND out "GtkConfigStore                : ")
  check_on_or_off("${NS3_GTK3}" "${GTK3_FOUND}")

  string(APPEND out "MPI Support                   : ")
  check_on_or_off("${NS3_MPI}" "${MPI_FOUND}")

  string(APPEND out "NS-3 Click Integration        : ")
  check_on_or_off("ON" "${NS3_CLICK}")

  string(APPEND out "NS-3 OpenFlow Integration     : ")
  check_on_or_off("ON" "${NS3_OPENFLOW}")

  string(APPEND out "Netmap emulation FdNetDevice  : ")
  check_on_or_off("${NS3_EMU}" "${ENABLE_NETMAP_EMU}")

  string(
    APPEND
    out
    "Network Simulation Cradle     : flag is set to ${NS3_NSC}, but currently not supported\n"
  )
  string(
    APPEND
    out
    "PlanetLab FdNetDevice         : flag is set to ${NS3_PLANETLAB}, but currently not supported\n"
  )
  string(APPEND out "PyViz visualizer              : ${NS3_VISUALIZER}\n")
  # string(APPEND out "Python API Scanning Support   : not enabled (castxml too
  # old)
  string(APPEND out "Python Bindings               : ${NS3_PYTHON_BINDINGS}\n")
  string(APPEND out "Real Time Simulator           : ")
  check_on_or_off("${NS3_REALTIME}" "${ENABLE_REALTIME}")

  string(APPEND out "SQLite stats support          : ")
  check_on_or_off("${NS3_SQLITE}" "${ENABLE_SQLITE}")

  string(APPEND out "Tap Bridge                    : ${NS3_TAP}\n")
  string(APPEND out "Tap FdNetDevice               : ")
  check_on_or_off("${NS3_TAP}" "${ENABLE_TAP}")

  string(APPEND out "Tests                         : ${TESTS_ENABLED}\n")
  string(APPEND out "Threading Primitives          : ")
  check_on_or_off("${NS3_PTHREAD}" "${THREADS_ENABLED}")

  # string(APPEND out "Use sudo to set suid bit      : not enabled (option
  # --enable-sudo not selected) string(APPEND out "XmlIo : enabled
  string(APPEND out "\n\n")

  set(really-enabled-modules ${ns3-libs};${ns3-contrib-libs})
  print_formatted_table_with_modules(
    "Modules that can be built" "${really-enabled-modules}" "out"
  )
  set(disabled-modules)
  foreach(module ${ns3-all-enabled-modules})
    if(NOT (lib${module} IN_LIST really-enabled-modules))
      list(APPEND disabled-modules ${module})
    endif()
  endforeach()
  print_formatted_table_with_modules(
    "Modules that cannot be built" "${disabled-modules}" "out"
  )

  file(WRITE ${PROJECT_BINARY_DIR}/ns3wafconfig.txt ${out})
  message(STATUS ${out})
endmacro()
