cmake_minimum_required(VERSION 3.10)

	set(NEPTUNE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Neptune") set(
		NTHREAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/NThread")

		if (NOT TARGET Neptune) if (
			EXISTS
			"${NEPTUNE_DIR}" AND EXISTS
			"${NEPTUNE_DIR}/CMakeLists.txt") add_subdirectory(${
			NEPTUNE_DIR }) else() message(STATUS
						      "Neptune directory or CMakeLists.txt not found, skipping add_subdirectory(Neptune).")
			endif() endif()

				if (NOT TARGET NThread) if (
					EXISTS
					"${NTHREAD_DIR}" AND EXISTS
					"${NTHREAD_DIR}/CMakeLists.txt") add_subdirectory(${
					NTHREAD_DIR }) else() message(STATUS
								      "NThread directory or CMakeLists.txt not found, skipping add_subdirectory(NThread).")
					endif() endif()

						if (TARGET Neptune AND TARGET NThread) add_executable(
							gadgets ${
								CMAKE_CURRENT_SOURCE_DIR } /
							gadgets.c) target_link_libraries(gadgets PRIVATE NThreadOSUtils
												 NThread Neptune)
							target_compile_definitions(
								gadgets PRIVATE LOG_LEVEL_3
									NEPTUNE_MODULERULES_HEADER =
										"ntosutils_rules.h")
								target_include_directories(
									gadgets PRIVATE ${
										NTHREAD_INCLUDE_DIR }) else()
									message(STATUS
										"Neptune or NThread target not found, skipping tests.")
										endif()
