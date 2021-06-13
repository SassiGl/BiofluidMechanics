## Build with Simbody and/or oneTBB source code
option(BUILD_WITH_DEPENDENCIES_SOURCE "BUILD_WITH_DEPENDENCIES_SOURCE" 0)

## Static build
option(SPH_ONLY_STATIC_BUILD "SPH_ONLY_STATIC_BUILD" 0)
if(SPH_ONLY_STATIC_BUILD)
    set(BUILD_SHARED_LIBS OFF)
    message(STATUS "SPH_ONLY_STATIC_BUILD is set on")
endif(SPH_ONLY_STATIC_BUILD)

## Webassembly ##
if(NOT DEFINED WASM_BUILD)
    set(WASM_BUILD 0)
endif(NOT DEFINED WASM_BUILD)

if(WASM_BUILD)
    message(STATUS "WEBASSEMBLY IS BEING BUILT...")
    set(BUILD_WITH_DEPENDENCIES_SOURCE 1)
    message(WARNING "SPH_ONLY_STATIC_BUILD is forced on")
    set(SPH_ONLY_STATIC_BUILD 1)
endif(WASM_BUILD)


if(BUILD_WITH_DEPENDENCIES_SOURCE)
    message(STATUS "BUILD_WITH_DEPENDENCIES_SOURCE")
    ## Select which dependency source code is included
    ###### Simbody ######
    # Simbody and clapack source code will be built with the project
    set(BUILD_WITH_SIMBODY 1)
    ###### Simbody ######
    ###### oneTBB ######
    # oneTBB source code will be built with the project
    set(BUILD_WITH_ONETBB 1)
    if(BUILD_WITH_ONETBB)
        add_definitions(-DTBB_2021_2_0)
    endif()
    ###### oneTBB ######
    ###### Boost, only 3D ######
    # select only 3D cases to build, if not, Boost is needed for 2D cases
    set(ONLY_3D 1)
    if(NOT ONLY_3D)
        add_definitions(-DBOOST_AVAILABLE)
        set(BOOST_AVAILABLE 1)
    endif()

else(BUILD_WITH_DEPENDENCIES_SOURCE)
    ## Default option, doesn't build any dependencies
    set(BUILD_WITH_SIMBODY 0)
    set(BUILD_WITH_ONETBB 0)
    add_definitions(-DBOOST_AVAILABLE)
    set(BOOST_AVAILABLE 1)
endif(BUILD_WITH_DEPENDENCIES_SOURCE)



###### Simbody ######
if(BUILD_WITH_SIMBODY)
    include_directories(${PLATFORM_INCLUDE_DIRECTORIES})
    set(SIMBODY_MAJOR_VERSION 3)
    set(SIMBODY_MINOR_VERSION 7)
    set(SIMBODY_PATCH_VERSION 0)
    set(PATCH_VERSION_STRING)
    if(SIMBODY_PATCH_VERSION)
        set(PATCH_VERSION_STRING ".${SIMBODY_PATCH_VERSION}")
    endif()
    set(SIMBODY_VERSION
        "${SIMBODY_MAJOR_VERSION}.${SIMBODY_MINOR_VERSION}${PATCH_VERSION_STRING}"
        CACHE STRING
        "This is the version that will be built (can't be changed in GUI)."
        FORCE)
    set(SIMBODY_SONAME_VERSION
        "${SIMBODY_MAJOR_VERSION}.${SIMBODY_MINOR_VERSION}"
        CACHE STRING
        "Soname version; appended to names of shared libs
        (can't be changed in GUI)."
        FORCE)
    set(VN "_${SIMBODY_VERSION}")
    set(SimTKCOMMON_MAJOR_VERSION ${SIMBODY_MAJOR_VERSION})
    set(SimTKCOMMON_MINOR_VERSION ${SIMBODY_MINOR_VERSION})
    set(SimTKCOMMON_PATCH_VERSION ${SIMBODY_PATCH_VERSION})
    set(SIMMATH_MAJOR_VERSION ${SIMBODY_MAJOR_VERSION})
    set(SIMMATH_MINOR_VERSION ${SIMBODY_MINOR_VERSION})
    set(SIMMATH_PATCH_VERSION ${SIMBODY_PATCH_VERSION})
    add_definitions(-DSimTK_SIMMATH_MAJOR_VERSION=${SIMMATH_MAJOR_VERSION}
                    -DSimTK_SIMMATH_MINOR_VERSION=${SIMMATH_MINOR_VERSION}
            -DSimTK_SIMMATH_PATCH_VERSION=${SIMMATH_PATCH_VERSION})
    add_definitions(-DSimTK_SIMBODY_MAJOR_VERSION=${SIMMATH_MAJOR_VERSION}
                    -DSimTK_SIMBODY_MINOR_VERSION=${SIMMATH_MINOR_VERSION}
            -DSimTK_SIMBODY_PATCH_VERSION=${SIMMATH_PATCH_VERSION})    
    add_definitions(-DSimTK_SimTKCOMMON_MAJOR_VERSION=${SimTKCOMMON_MAJOR_VERSION}
            -DSimTK_SimTKCOMMON_MINOR_VERSION=${SimTKCOMMON_MINOR_VERSION}
            -DSimTK_SimTKCOMMON_PATCH_VERSION=${SimTKCOMMON_PATCH_VERSION})

    add_definitions(-DBUILD_VISUALIZER=off)
    set(CMAKE_C_FLAGS "-DINTEGER_STAR_8")
endif(BUILD_WITH_SIMBODY)
###### Simbody ######