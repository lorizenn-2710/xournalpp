cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

file(GLOB_RECURSE xournalpp_sources *.cpp *.h)

if(NOT ENABLE_AUDIO)
    list(FILTER xournalpp_sources EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/core/audio/.*" )
    list(FILTER xournalpp_sources EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/core/control/AudioController.cpp" )
endif()
set(xournalpp_include_dirs "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(xournalpp-core STATIC ${xournalpp_sources})
target_compile_features(xournalpp-core PUBLIC ${PROJECT_CXX_FEATURES})
target_compile_features(xournalpp-core PUBLIC cxx_std_20)
target_link_libraries(xournalpp-core
        PRIVATE xoj::util ${xournalpp_LDFLAGS}
        PUBLIC xoj::defaults xoj::external_modules
        )
target_include_directories(xournalpp-core PUBLIC ${xournalpp_include_dirs})
add_library(xoj::core ALIAS xournalpp-core)

# Exact decimal polynomial algebra for certified teaching variation tables.
find_library(TEACHING_GMPXX NAMES gmpxx REQUIRED)
find_library(TEACHING_GMP NAMES gmp REQUIRED)
target_link_libraries(xournalpp-core PRIVATE ${TEACHING_GMPXX} ${TEACHING_GMP})

# Installed local OCR worker, independent of build tree and process CWD.
target_compile_definitions(xournalpp-core PRIVATE
    TEACHING_OCR_PYTHON="${CMAKE_INSTALL_PREFIX}/share/xournalpp/formula-ocr/bin/python"
    TEACHING_OCR_SCRIPT="${CMAKE_INSTALL_PREFIX}/share/xournalpp/teaching-tools/formula-ocr.py")
install(PROGRAMS "${PROJECT_SOURCE_DIR}/scripts/teaching-tools/formula-ocr.py"
    "${PROJECT_SOURCE_DIR}/scripts/teaching-tools/setup-formula-ocr.sh"
    DESTINATION share/xournalpp/teaching-tools)

install(FILES "${PROJECT_SOURCE_DIR}/scripts/teaching-tools/requirements-ocr.lock.txt"
    "${PROJECT_SOURCE_DIR}/scripts/teaching-tools/models-sha256.json"
    DESTINATION share/xournalpp/teaching-tools)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/scripts/teaching-tools/licenses/"
    DESTINATION share/xournalpp/teaching-tools/licenses)
