cmake_minimum_required(VERSION 3.10)

project(quiz CXX)

add_compile_options(-Wall -std=c++17)

if (debug)
	add_compile_options(-g -O0 -DDEBUG)
endif()

include_directories(
	include
)

# ********************** shared ************************
add_library(analyze_lib OBJECT src/analyzer.cpp)
add_library(utils_lib OBJECT src/utils.cpp)

# *********************** quiz ************************
add_executable(quiz "")

target_sources(quiz
	PRIVATE
		src/log.cpp
		src/parser.cpp
		src/quiz.cpp
		src/voice.cpp
		src/view/ncurses/editor.cpp
		src/view/ncurses/ncurses_screen.cpp
		src/view/ncurses/window.cpp
		$<TARGET_OBJECTS:analyze_lib>
		$<TARGET_OBJECTS:utils_lib>
)

target_link_libraries(quiz
	PRIVATE
		ncursesw
		stdc++fs
)

if (audio)
	target_compile_options(quiz PUBLIC -DAUDIO_CAPTURE)
	target_include_directories(quiz
		PRIVATE
			/usr/include/pocketsphinx
			/usr/include/x86_64-linux-gnu/sphinxbase
	)
	target_link_libraries(quiz
		PRIVATE
			pocketsphinx
			sphinxbase
			sphinxad
	)
endif()

#if (spd-say)
#	target_compile_options(quiz PUBLIC -DSPD_SAY)
#elseif (google_speech)
	target_compile_options(quiz PUBLIC -DGOOGLE_SPEECH)
#endif()


# *********************** tests ************************

#set GTEST_DIR variable to  "your_path/Gtest/googletest"
if (GTEST_DIR)
	add_executable(analyze-test "")
	add_dependencies(quiz run_test)
	add_custom_target(run_test ALL ./analyze-test DEPENDS analyze-test)

	target_compile_options(analyze-test PRIVATE -pthread)

	target_include_directories(analyze-test
		PRIVATE
			${GTEST_DIR}
			${GTEST_DIR}/include
	)

	target_sources(analyze-test
		PRIVATE
			${GTEST_DIR}/src/gtest-all.cc
			test/analyzer-test.cpp
			$<TARGET_OBJECTS:analyze_lib>
			$<TARGET_OBJECTS:utils_lib>
	)

	target_link_libraries(analyze-test
		PRIVATE
			pthread
	)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

