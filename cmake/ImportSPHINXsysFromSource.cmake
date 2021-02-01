FILE(GLOB_RECURSE SPHINXsysHeaderPathList ${CMAKE_SOURCE_DIR}/SPHINXsys/src/*.h)

SET(SPHINXsysHeaderPath "")
FOREACH(file_path ${SPHINXsysHeaderPathList})
    GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
    SET(SPHINXsysHeaderPath ${SPHINXsysHeaderPath} ${dir_path})
ENDFOREACH()
LIST(REMOVE_DUPLICATES SPHINXsysHeaderPath)

INCLUDE_DIRECTORIES("${SPHINXsysHeaderPath}")
LINK_DIRECTORIES("${CMAKE_BINARY_DIR}/SPHINXsys/lib")
LINK_DIRECTORIES("${CMAKE_BINARY_DIR}/SPHINXsys/bin")