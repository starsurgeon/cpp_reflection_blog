
# Build the std::meta Part 2 examples.
# Note: these examples are written as headers containing `main()`. We compile
# them as translation units by forcing their language to CXX.

set(_std_meta_part2_examples
	template_for
	namespace_reflection
	struct_reflection
)

foreach(_ex IN LISTS _std_meta_part2_examples)
	set(_src "${CMAKE_CURRENT_SOURCE_DIR}/include/${_ex}.h")
	set_source_files_properties("${_src}" PROPERTIES LANGUAGE CXX)
	add_reflected_executable("${_ex}" "${_src}")
endforeach()

function(_std_meta_part2_configure_clang_libcxx target)
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		return()
	endif()

	# Prefer the devtoolchain's libc++ headers if available.
	if(EXISTS "/opt/clang-p2996/include/c++/v1")
		target_include_directories(${target} PRIVATE "/opt/clang-p2996/include/c++/v1")
		target_include_directories(${target} PRIVATE "/opt/clang-p2996/include/x86_64-unknown-linux-gnu/c++/v1")
	endif()

	# Prefer the bundled libc++ implementation and avoid system libstdc++ headers.
	if(EXISTS "/opt/clang-p2996/include/c++/v1")
		target_compile_options(${target} PRIVATE -nostdinc++)
	endif()

	# Add runtime/linker flags for the toolchain libc++ location.
	if(EXISTS "/opt/clang-p2996/lib/x86_64-unknown-linux-gnu")
		target_link_options(${target} PRIVATE
			"-L/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
			"-Wl,-rpath,/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
			"-lc++" "-lc++abi")
	elseif(EXISTS "/opt/clang-p2996/lib")
		target_link_options(${target} PRIVATE
			"-L/opt/clang-p2996/lib"
			"-Wl,-rpath,/opt/clang-p2996/lib"
			"-lc++" "-lc++abi")
	endif()
endfunction()

foreach(_ex IN LISTS _std_meta_part2_examples)
	_std_meta_part2_configure_clang_libcxx(${_ex})
	install(TARGETS ${_ex} RUNTIME DESTINATION bin)
endforeach()

