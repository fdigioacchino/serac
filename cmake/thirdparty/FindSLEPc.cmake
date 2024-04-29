# Copyright (c) 2024, Lawrence Livermore National Security, LLC. All rights
# reserved.  LLNL-CODE-856946

# OFFICIAL USE ONLY This work was produced at the Lawrence Livermore National
# Laboratory (LLNL) under contract no. DE-AC52-07NA27344 (Contract 44) between
# the U.S. Department of Energy (DOE) and Lawrence Livermore National Security,
# LLC (LLNS) for the operation of LLNL.  See license for disclaimers, notice of
# U.S. Government Rights and license terms and conditions.

# Defines the following imported target: PkgConfig::SLEPC

find_package(PkgConfig REQUIRED)

find_path(
  SLEPC_PKG_CONFIG "SLEPc.pc"
  PATHS ${SLEPC_DIR}
  PATH_SUFFIXES lib/pkgconfig/)

if(SLEPC_PKG_CONFIG)
  set(ENV{PKG_CONFIG_PATH} "${SLEPC_PKG_CONFIG}:$ENV{PKG_CONFIG_PATH}")
endif()

pkg_search_module(SLEPC REQUIRED IMPORTED_TARGET SLEPc)

if(NOT ARPACK_DIR)
  message(FATAL_ERROR "Could not find arpack. Slepc requires ARPACK_DIR.")
endif()

# Add petsc and arpack link libraries
get_target_property(_slepc_link_libs PkgConfig::SLEPC INTERFACE_LINK_LIBRARIES)
get_target_property(_petsc_link_libs PkgConfig::PETSC INTERFACE_LINK_LIBRARIES)
list(APPEND _slepc_link_libs ${_petsc_link_libs})

# NOTE: The arpack shared object file may be at a different location depending on system
set(_libparpack_paths ${ARPACK_DIR}/lib/libparpack.so ${ARPACK_DIR}/lib64/libparpack.so)
foreach(_path ${_libparpack_paths})
  if (EXISTS ${_path})
    list(APPEND _slepc_link_libs ${_path})
  endif()
endforeach()

blt_list_remove_duplicates(TO _slepc_link_libs)
set_target_properties(PkgConfig::SLEPC PROPERTIES INTERFACE_LINK_LIBRARIES "${_slepc_link_libs}")

# Add arpack include directories
target_include_directories(PkgConfig::SLEPC INTERFACE ${ARPACK_DIR}/include)

# Add petsc link directories
get_target_property(_petsc_link_dirs PkgConfig::PETSC INTERFACE_LINK_DIRECTORIES)
target_link_directories(PkgConfig::SLEPC INTERFACE ${_petsc_link_dirs})
