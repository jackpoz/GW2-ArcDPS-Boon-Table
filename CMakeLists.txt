cmake_minimum_required(VERSION 3.25)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("GW2-ArcDPS-Boon-Table")

option(ASAN "Build with AddressSanitizer" OFF)
option(TSAN "Build with ThreadSanitizer" OFF)

if(ASAN AND TSAN)
  message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be enabled together")
endif()

if(ASAN OR TSAN)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Sanitizers require Clang/clang-cl")
  endif()

  set(_san_flags -fno-omit-frame-pointer)
  if(ASAN)
    list(APPEND _san_flags -fsanitize=address -fsanitize-recover=address -fsanitize-address-use-after-scope)
    message(STATUS "Clang: Enabled Address Sanitizer ASan")
  endif()
  if(TSAN)
    list(APPEND _san_flags -fsanitize=thread)
    message(STATUS "Clang: Enabled Thread Sanitizer TSan")
  endif()

  add_compile_options(${_san_flags})
  add_link_options(${_san_flags})
endif()

add_subdirectory ("ArcDPS Boon Table")
