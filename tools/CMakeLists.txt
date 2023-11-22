set(LIBRARIES ${LIBRARIES} ${CORE_LIB})

add_executable(x2kahip x2kahip.cpp)
target_link_libraries(x2kahip ${LIBRARIES} implementation-core)

add_executable(metrics metrics.cpp)
target_link_libraries(metrics ${LIBRARIES} implementation-core)

add_executable(graphgen graphgen.cpp)
target_link_libraries(graphgen ${LIBRARIES})

add_executable(permute permute.cpp)
target_link_libraries(permute ${LIBRARIES})