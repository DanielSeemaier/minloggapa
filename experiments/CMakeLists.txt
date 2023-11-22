set(LIBRARIES ${LIBRARIES} ${CORE_LIB})

add_executable(basic_fm_mix basic_fm_mix.cpp ${SOURCE_FILES})
target_link_libraries(basic_fm_mix ${LIBRARIES})

add_executable(ls_improvement_test ls_improvement_test.cpp)
target_link_libraries(ls_improvement_test ${LIBRARIES})

