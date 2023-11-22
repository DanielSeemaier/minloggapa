set(SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/initial-partitioner/initial_partitioner_interface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/initial-partitioner/random_initial_partitioner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/initial-partitioner/random_initial_partitioner.h
        ${CMAKE_CURRENT_SOURCE_DIR}/initial-partitioner/kahip_initial_partitioner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/initial-partitioner/kahip_initial_partitioner.h
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/refiner_interface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/refiner_interface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/basic_refiner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/basic_refiner.h
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/fm_refiner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/fm_refiner.h
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/fm_refiner_quadtree.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/refinement/fm_refiner_quadtree.h
        ${CMAKE_CURRENT_SOURCE_DIR}/data-structure/query_graph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/data-structure/query_graph.h
        ${CMAKE_CURRENT_SOURCE_DIR}/report/reporter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/report/reporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/report/sqlite_reporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/report/sqlite_reporter.h)

add_library(${CORE_LIB} ${SOURCE_FILES})
target_link_libraries(${CORE_LIB} ${LIBRARIES})
target_include_directories(${CORE_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})