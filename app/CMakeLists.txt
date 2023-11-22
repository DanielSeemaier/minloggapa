set(LIBRARIES ${LIBRARIES} ${CORE_LIB})

add_executable(minloggapa minloggapa.cpp ${SOURCE_FILES})
target_link_libraries(minloggapa ${LIBRARIES})


