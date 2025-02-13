add_executable(
  test-all
  Common/main.cpp
  Disk.cpp
  Decomposition.cpp
  LinearAlgebra.cpp
  Other.cpp
  Polynomial.cpp
  Tensor.cpp
  TensorAlgebra.cpp
  TensorAlgebraTasks.cpp
  BlockTensor.cpp
  ElementOps.cpp
  TiledTensor.cpp
  QChemTests.cpp
  OtherCatch.cpp
  ArithmeticTensor.cpp
  FunctionTensor.cpp
  RuntimeTensors.cpp
  InitFinalize.cpp
)

set_target_properties(
  test-all PROPERTIES BUILD_RPATH "${_LIB_RPATH};${CMAKE_BUILD_RPATH}"
                      INSTALL_RPATH "${_LIB_RPATH};${CMAKE_INSTALL_RPATH};${_LIB_RPATH}/../../lib"
)

file(GLOB __data_files data/water_sto3g/*)
file(COPY ${__data_files} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data/water_sto3g)

if(EINSUMS_WITH_COVERAGE AND NOT MSVC)
  target_compile_options(
    test-all PUBLIC $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-fprofile-instr-generate -fcoverage-mapping>
  )
  target_compile_options(
    test-all PUBLIC $<$<COMPILE_LANGUAGE:HIP>:-fprofile-instr-generate -fcoverage-mapping>
  )
  target_compile_options(test-all PUBLIC $<$<COMPILE_LANG_AND_ID:CXX,GNU>:--coverage>)
  target_link_options(
    test-all PUBLIC $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-fprofile-instr-generate -fcoverage-mapping>
  )
  target_link_options(
    test-all PUBLIC $<$<COMPILE_LANGUAGE:HIP>:-fprofile-instr-generate -fcoverage-mapping>
  )
  target_link_options(test-all PUBLIC $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-lgcov --coverage>)
endif()

if(NOT MSVC)
  target_compile_options(einsums BEFORE PUBLIC $<$<CONFIG:Debug>:-gdwarf-2>)
endif()

extend_einsums_target(test-all SOURCES FFT.cpp CONDITION TARGET FFT::FFT)

extend_einsums_target(
  test-all
  SOURCES
  DeviceTensor_extras.hip
  DeviceTensor.hip
  DeviceTensorView.hip
  DeviceTiledTensor.hip
  GPULinearAlgebra.hip
  GPU.hip
  GPUTensorAlgebra.hip
  GPUTensorAlgebraTasks.hip
  BlockDeviceTensor.hip
  ElementOpsGPU.hip
  CONDITION
  EINSUMS_BUILD_HIP
  PUBLIC_DEPENDS
  roc::hipblas
  roc::hipsolver
  hip::host
  hip::device
)

# Enable einsum auto test capabilities
target_compile_definitions(
  test-all PRIVATE EINSUMS_USE_CATCH2 EINSUMS_CONTINUOUSLY_TEST_EINSUM
                   TEST_PATH="${CMAKE_CURRENT_LIST_DIR}"
)
find_package(Catch2)

target_link_libraries(test-all PUBLIC Catch2::Catch2 einsums)

include(CTest)
include(Catch)
catch_discover_tests(test-all)

install(TARGETS test-all RUNTIME DESTINATION ${EINSUMS_DATA_PATH})

install(FILES ${__data_files} DESTINATION ${EINSUMS_DATA_PATH}/data/water_sto3g)
