cmake_minimum_required (VERSION 3.8)

project ("NativeTech")

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE on)

set (ROOT_PROJECT_DIRECTORY "${CMAKE_SOURCE_DIR}/..")

set (ABS_PRECOMPUTE_DATA_DIRECTORY "${ROOT_PROJECT_DIRECTORY}/data/PrecomputeData")

list(APPEND EmdeddedFiles "${ABS_PRECOMPUTE_DATA_DIRECTORY}/preflop_precompute_16_13.bin")

if (EMSCRIPTEN)
	set (SUPPORT_THREADS false)
else()
	set (SUPPORT_THREADS true)
endif()

if (SUPPORT_THREADS)
	add_compile_definitions(SUPPORT_THREADS)
	find_package( Threads )
endif()

add_library (PokerTech)
set(POKER_TECH_SOURCES
	PokerTech/EquityCalculator/EquityCalculator.cpp	
	PokerTech/Precompute/PreflopPrecomputeTable.cpp
	PokerTech/Precompute/PreflopPrecomputeTableBuilder.cpp
	PokerTech/RangeConverters/EnabledRangeToSyntax.cpp
	PokerTech/RangeConverters/RangeBuilderPercentage.cpp
	PokerTech/RangeConverters/RangeBuilderSyntaxString.cpp
	PokerTech/ComboUtilities.cpp
	PokerTech/HandRange.cpp
	PokerTech/HandRanking.cpp
	PokerTech/pch.cpp
	PokerTech/PokerTech.cpp
	PokerTech/PokerUtilities.cpp
	PokerTech/StringUtility.cpp
	PokerTech/Utilities.cpp
	PokerTech/EnabledRange.cpp
)

target_link_libraries( PokerTech ${CMAKE_THREAD_LIBS_INIT} )

file(GLOB_RECURSE POKER_TECH_HEADER_SOURCES PokerTech/*.h)

target_sources(PokerTech PRIVATE ${POKER_TECH_SOURCES} ${POKER_TECH_HEADER_SOURCES})
target_include_directories(PokerTech PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# enable this and intellisense will work
#target_include_directories(PokerTech PUBLIC $ENV{EMSDK}/upstream/emscripten/system/include)

# executable for creating precomputed data
if (NOT EMSCRIPTEN)
	add_executable (PokerPrecompute)
	target_sources(PokerPrecompute PRIVATE
		Precompute/PokerPrecompute.cpp
		Precompute/PrecomputeTableBuilder.h
	)

	target_link_libraries(PokerPrecompute PokerTech)
	add_compile_definitions(TARGET PokerPrecompute PRECOMPUTE_DIRECTORY="${ABS_PRECOMPUTE_DATA_DIRECTORY}")
endif()

if (BUILD_TESTS)

	add_executable (PokerTests)

	add_test(NAME AllTests COMMAND PokerTests)

	set(POKER_TEST_SOURCES
		Tests/Deck.cpp
		Tests/ComboUtilitiesTests.cpp
		Tests/EquityCalculatorTests.cpp
		Tests/HandRankingTests.cpp
		Tests/HandSyntaxTests.cpp
		Tests/PokerUtilitiesTests.cpp
		Tests/pch.cpp
		Tests/PreflopPrecomputeTests.cpp
		Tests/HandRangeTests.cpp
		Tests/TestEnvironment.cpp
	)
	file(GLOB POKER_TEST_HEADER_SOURCES Tests/*.h)
	target_sources(PokerTests PRIVATE ${POKER_TEST_SOURCES} ${POKER_TEST_HEADER_SOURCES})

	include(FetchContent)
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG release-1.12.1
	)
	# For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)

	target_link_libraries(PokerTests PokerTech GTest::gtest_main)

endif()

foreach(embeddedPath ${EmdeddedFiles})
	cmake_path(GET embeddedPath FILENAME filename)
	configure_file(${embeddedPath} ${CMAKE_BINARY_DIR}/${filename} COPYONLY)
endforeach(embeddedPath)

if (EMSCRIPTEN)

	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

	add_compile_definitions(EMSCRIPTEN)

	add_executable(PokerTechWasm)
	target_sources(PokerTechWasm PRIVATE
		PokerTechWasm/WasmInterface.cpp
		PokerTechWasm/TypesToJson.cpp
	)
	target_link_libraries(PokerTechWasm PokerTech)

	#target_link_options(PokerTechWasm PRIVATE -sEXPORTED_FUNCTIONS=['_TestFunction'])
	target_link_options(PokerTechWasm PRIVATE -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','allocate','intArrayFromString'])
	target_link_options(PokerTechWasm PRIVATE -sEXPORT_NAME='createModule')


	if (NOT BUILD_FOR_NODE)
		#if (SUPPORT_THREADS)
			#target_link_options(PokerTechWasm PRIVATE -sENVIRONMENT='web,worker')
		#else()
			#target_link_options(PokerTechWasm PRIVATE -sENVIRONMENT='web')
		#	endif()
			#target_link_options(PokerTechWasm PRIVATE -sENVIRONMENT='web')
			#target_link_options(PokerTechWasm PRIVATE -sEXPORT_ES6=1)
			#target_link_options(PokerTechWasm PRIVATE -sUSE_ES6_IMPORT_META=0)
		#else()

	endif()

	target_link_options(PokerTechWasm PRIVATE -sMODULARIZE)
	#target_link_options(PokerTechWasm PRIVATE -sSINGLE_FILE=1)
	target_link_options(PokerTechWasm PRIVATE -lembind)
	target_link_options(PokerTechWasm PRIVATE -sASYNCIFY)
	target_link_options(PokerTechWasm PRIVATE -sNO_DISABLE_EXCEPTION_CATCHING)
	target_link_options(PokerTechWasm PRIVATE -O3)

	if (SUPPORT_THREADS)
		target_link_options(PokerTechWasm PRIVATE -sUSE_PTHREADS=1)
		target_link_options(PokerTechWasm PRIVATE -sALLOW_MEMORY_GROWTH)
		target_link_options(PokerTechWasm PRIVATE -sPTHREAD_POOL_SIZE=12)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	endif()

	#set_target_properties(PokerTechWasm PROPERTIES -pthread)
	#set_target_properties(PokerTechWasm PROPERTIES LINK_FLAGS "--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/jswrapper/locateFile.js")
	set_target_properties(PokerTechWasm PROPERTIES SUFFIX ".js")
	#set_target_properties(PokerTechWasm PROPERTIES LINK_FLAGS "-v --embed-file preflop_precompute_16_13.bin")

	#COMMON_CFLAGS += -pthread
	#COMMON_CXXFLAGS += -pthread

	foreach(embeddedPath ${EmdeddedFiles})
		cmake_path(GET embeddedPath FILENAME filename)
		set_target_properties(PokerTechWasm PROPERTIES LINK_FLAGS "--embed-file ${filename}")
	endforeach(embeddedPath)

	# Put the generated file into the js/src dir. This is where the API js code is located
	add_custom_command(TARGET PokerTechWasm POST_BUILD 
		COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:PokerTechWasm> ${ROOT_PROJECT_DIRECTORY}/js/src)

endif()

include(CMakeListsUser.cmake OPTIONAL)