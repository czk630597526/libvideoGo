# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/chenzk/BaseLib/BaseLib/libvideoGo

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go

# Utility rule file for media_go_swig_compilation.

# Include the progress variables for this target.
include CMakeFiles/media_go_swig_compilation.dir/progress.make

CMakeFiles/media_go_swig_compilation: CMakeFiles/media_go.dir/libvideoGO.stamp


CMakeFiles/media_go.dir/libvideoGO.stamp: ../libvideo.i
CMakeFiles/media_go.dir/libvideoGO.stamp: ../libvideo.i
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Swig compile libvideo.i for go"
	/usr/bin/cmake -E make_directory /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/go/Lib_media /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/go/Lib_media
	/usr/bin/cmake -E touch /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles/media_go.dir/libvideoGO.stamp
	/usr/bin/cmake -E env SWIG_LIB=/usr/share/swig4.0 /usr/bin/swig4.0 -go -cgo -intgosize 64 -I./include -I/home/chenzk/BaseLib/BaseLib/libvideoGo/./openh264_inc/ -I/home/chenzk/BaseLib/BaseLib/libvideoGo/./ffm_inc_new/ -I/home/chenzk/BaseLib/BaseLib/libvideoGo/./include -I/home/chenzk/BaseLib/BaseLib/libvideoGo/./openh264_inc/ -I/home/chenzk/BaseLib/BaseLib/libvideoGo/./ffm_inc_new/ -I/home/chenzk/BaseLib/BaseLib/libvideoGo/./include -package libvideo -outdir /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/go/Lib_media -o /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/go/Lib_media/libvideoGO_wrap.c /home/chenzk/BaseLib/BaseLib/libvideoGo/libvideo.i

media_go_swig_compilation: CMakeFiles/media_go_swig_compilation
media_go_swig_compilation: CMakeFiles/media_go.dir/libvideoGO.stamp
media_go_swig_compilation: CMakeFiles/media_go_swig_compilation.dir/build.make

.PHONY : media_go_swig_compilation

# Rule to build all files generated by this target.
CMakeFiles/media_go_swig_compilation.dir/build: media_go_swig_compilation

.PHONY : CMakeFiles/media_go_swig_compilation.dir/build

CMakeFiles/media_go_swig_compilation.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/media_go_swig_compilation.dir/cmake_clean.cmake
.PHONY : CMakeFiles/media_go_swig_compilation.dir/clean

CMakeFiles/media_go_swig_compilation.dir/depend:
	cd /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chenzk/BaseLib/BaseLib/libvideoGo /home/chenzk/BaseLib/BaseLib/libvideoGo /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles/media_go_swig_compilation.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/media_go_swig_compilation.dir/depend
