project(boost_python)

set(BP_SOURCES
  src/dict.cpp
  src/errors.cpp
  src/exec.cpp
  src/import.cpp
  src/list.cpp
  src/long.cpp
  src/module.cpp
  src/numeric.cpp
  src/object_operators.cpp
  src/object_protocol.cpp
  src/slice.cpp
  src/str.cpp
  src/tuple.cpp
  src/wrapper.cpp
  src/converter/arg_to_python_base.cpp
  src/converter/builtin_converters.cpp
  src/converter/from_python.cpp
  src/converter/registry.cpp
  src/converter/type_id.cpp
  src/object/class.cpp
  src/object/enum.cpp
  src/object/function.cpp
  src/object/function_doc_signature.cpp
  src/object/inheritance.cpp
  src/object/iterator.cpp
  src/object/life_support.cpp
  src/object/pickle_support.cpp
  src/object/stl_iterator.cpp
)

add_library(boost_python STATIC ${BP_SOURCES})
add_dependencies(boost_python python)

target_include_directories(boost_python PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/")

target_compile_definitions(boost_python PUBLIC
  BOOST_AUTO_LINK_NOMANGLE
  BOOST_PYTHON_STATIC_LIB
  BOOST_PYTHON_NO_PY_SIGNATURES
  BOOST_SERIALIZATION_NO_LIB
  BOOST_PYTHON_MAX_ARITY=20
)

if(CMAKE_HOST_UNIX)
  target_compile_options(boost_python PRIVATE "-fPIC")
endif()
