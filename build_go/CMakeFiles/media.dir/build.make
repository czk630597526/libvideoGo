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

# Include any dependencies generated for this target.
include CMakeFiles/media.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/media.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/media.dir/flags.make

CMakeFiles/media.dir/libvideo.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/libvideo.c.o: ../libvideo.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/media.dir/libvideo.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/libvideo.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/libvideo.c

CMakeFiles/media.dir/libvideo.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/libvideo.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/libvideo.c > CMakeFiles/media.dir/libvideo.c.i

CMakeFiles/media.dir/libvideo.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/libvideo.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/libvideo.c -o CMakeFiles/media.dir/libvideo.c.s

CMakeFiles/media.dir/h264_rtp.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/h264_rtp.c.o: ../h264_rtp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/media.dir/h264_rtp.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/h264_rtp.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_rtp.c

CMakeFiles/media.dir/h264_rtp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/h264_rtp.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_rtp.c > CMakeFiles/media.dir/h264_rtp.c.i

CMakeFiles/media.dir/h264_rtp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/h264_rtp.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_rtp.c -o CMakeFiles/media.dir/h264_rtp.c.s

CMakeFiles/media.dir/libv_log.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/libv_log.c.o: ../libv_log.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/media.dir/libv_log.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/libv_log.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/libv_log.c

CMakeFiles/media.dir/libv_log.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/libv_log.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/libv_log.c > CMakeFiles/media.dir/libv_log.c.i

CMakeFiles/media.dir/libv_log.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/libv_log.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/libv_log.c -o CMakeFiles/media.dir/libv_log.c.s

CMakeFiles/media.dir/h264_ps.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/h264_ps.c.o: ../h264_ps.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/media.dir/h264_ps.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/h264_ps.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_ps.c

CMakeFiles/media.dir/h264_ps.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/h264_ps.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_ps.c > CMakeFiles/media.dir/h264_ps.c.i

CMakeFiles/media.dir/h264_ps.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/h264_ps.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_ps.c -o CMakeFiles/media.dir/h264_ps.c.s

CMakeFiles/media.dir/base64.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/base64.c.o: ../base64.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/media.dir/base64.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/base64.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/base64.c

CMakeFiles/media.dir/base64.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/base64.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/base64.c > CMakeFiles/media.dir/base64.c.i

CMakeFiles/media.dir/base64.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/base64.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/base64.c -o CMakeFiles/media.dir/base64.c.s

CMakeFiles/media.dir/libv_stat.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/libv_stat.c.o: ../libv_stat.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/media.dir/libv_stat.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/libv_stat.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/libv_stat.c

CMakeFiles/media.dir/libv_stat.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/libv_stat.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/libv_stat.c > CMakeFiles/media.dir/libv_stat.c.i

CMakeFiles/media.dir/libv_stat.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/libv_stat.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/libv_stat.c -o CMakeFiles/media.dir/libv_stat.c.s

CMakeFiles/media.dir/video_mix.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/video_mix.c.o: ../video_mix.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/media.dir/video_mix.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/video_mix.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/video_mix.c

CMakeFiles/media.dir/video_mix.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/video_mix.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/video_mix.c > CMakeFiles/media.dir/video_mix.c.i

CMakeFiles/media.dir/video_mix.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/video_mix.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/video_mix.c -o CMakeFiles/media.dir/video_mix.c.s

CMakeFiles/media.dir/ffm_decoder.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/ffm_decoder.c.o: ../ffm_decoder.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/media.dir/ffm_decoder.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/ffm_decoder.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/ffm_decoder.c

CMakeFiles/media.dir/ffm_decoder.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/ffm_decoder.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/ffm_decoder.c > CMakeFiles/media.dir/ffm_decoder.c.i

CMakeFiles/media.dir/ffm_decoder.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/ffm_decoder.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/ffm_decoder.c -o CMakeFiles/media.dir/ffm_decoder.c.s

CMakeFiles/media.dir/h264_decoder.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/h264_decoder.c.o: ../h264_decoder.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/media.dir/h264_decoder.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/h264_decoder.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_decoder.c

CMakeFiles/media.dir/h264_decoder.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/h264_decoder.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_decoder.c > CMakeFiles/media.dir/h264_decoder.c.i

CMakeFiles/media.dir/h264_decoder.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/h264_decoder.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_decoder.c -o CMakeFiles/media.dir/h264_decoder.c.s

CMakeFiles/media.dir/h264_encoder.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/h264_encoder.c.o: ../h264_encoder.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object CMakeFiles/media.dir/h264_encoder.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/h264_encoder.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_encoder.c

CMakeFiles/media.dir/h264_encoder.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/h264_encoder.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_encoder.c > CMakeFiles/media.dir/h264_encoder.c.i

CMakeFiles/media.dir/h264_encoder.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/h264_encoder.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/h264_encoder.c -o CMakeFiles/media.dir/h264_encoder.c.s

CMakeFiles/media.dir/openh264_encoder.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/openh264_encoder.c.o: ../openh264_encoder.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object CMakeFiles/media.dir/openh264_encoder.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/openh264_encoder.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/openh264_encoder.c

CMakeFiles/media.dir/openh264_encoder.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/openh264_encoder.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/openh264_encoder.c > CMakeFiles/media.dir/openh264_encoder.c.i

CMakeFiles/media.dir/openh264_encoder.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/openh264_encoder.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/openh264_encoder.c -o CMakeFiles/media.dir/openh264_encoder.c.s

CMakeFiles/media.dir/openh264_decoder.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/openh264_decoder.c.o: ../openh264_decoder.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object CMakeFiles/media.dir/openh264_decoder.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/openh264_decoder.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/openh264_decoder.c

CMakeFiles/media.dir/openh264_decoder.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/openh264_decoder.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/openh264_decoder.c > CMakeFiles/media.dir/openh264_decoder.c.i

CMakeFiles/media.dir/openh264_decoder.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/openh264_decoder.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/openh264_decoder.c -o CMakeFiles/media.dir/openh264_decoder.c.s

CMakeFiles/media.dir/pic_sws.c.o: CMakeFiles/media.dir/flags.make
CMakeFiles/media.dir/pic_sws.c.o: ../pic_sws.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object CMakeFiles/media.dir/pic_sws.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/media.dir/pic_sws.c.o   -c /home/chenzk/BaseLib/BaseLib/libvideoGo/pic_sws.c

CMakeFiles/media.dir/pic_sws.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/media.dir/pic_sws.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chenzk/BaseLib/BaseLib/libvideoGo/pic_sws.c > CMakeFiles/media.dir/pic_sws.c.i

CMakeFiles/media.dir/pic_sws.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/media.dir/pic_sws.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chenzk/BaseLib/BaseLib/libvideoGo/pic_sws.c -o CMakeFiles/media.dir/pic_sws.c.s

# Object files for target media
media_OBJECTS = \
"CMakeFiles/media.dir/libvideo.c.o" \
"CMakeFiles/media.dir/h264_rtp.c.o" \
"CMakeFiles/media.dir/libv_log.c.o" \
"CMakeFiles/media.dir/h264_ps.c.o" \
"CMakeFiles/media.dir/base64.c.o" \
"CMakeFiles/media.dir/libv_stat.c.o" \
"CMakeFiles/media.dir/video_mix.c.o" \
"CMakeFiles/media.dir/ffm_decoder.c.o" \
"CMakeFiles/media.dir/h264_decoder.c.o" \
"CMakeFiles/media.dir/h264_encoder.c.o" \
"CMakeFiles/media.dir/openh264_encoder.c.o" \
"CMakeFiles/media.dir/openh264_decoder.c.o" \
"CMakeFiles/media.dir/pic_sws.c.o"

# External object files for target media
media_EXTERNAL_OBJECTS =

../lib/libmedia.so: CMakeFiles/media.dir/libvideo.c.o
../lib/libmedia.so: CMakeFiles/media.dir/h264_rtp.c.o
../lib/libmedia.so: CMakeFiles/media.dir/libv_log.c.o
../lib/libmedia.so: CMakeFiles/media.dir/h264_ps.c.o
../lib/libmedia.so: CMakeFiles/media.dir/base64.c.o
../lib/libmedia.so: CMakeFiles/media.dir/libv_stat.c.o
../lib/libmedia.so: CMakeFiles/media.dir/video_mix.c.o
../lib/libmedia.so: CMakeFiles/media.dir/ffm_decoder.c.o
../lib/libmedia.so: CMakeFiles/media.dir/h264_decoder.c.o
../lib/libmedia.so: CMakeFiles/media.dir/h264_encoder.c.o
../lib/libmedia.so: CMakeFiles/media.dir/openh264_encoder.c.o
../lib/libmedia.so: CMakeFiles/media.dir/openh264_decoder.c.o
../lib/libmedia.so: CMakeFiles/media.dir/pic_sws.c.o
../lib/libmedia.so: CMakeFiles/media.dir/build.make
../lib/libmedia.so: CMakeFiles/media.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Linking C shared library ../lib/libmedia.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/media.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/media.dir/build: ../lib/libmedia.so

.PHONY : CMakeFiles/media.dir/build

CMakeFiles/media.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/media.dir/cmake_clean.cmake
.PHONY : CMakeFiles/media.dir/clean

CMakeFiles/media.dir/depend:
	cd /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chenzk/BaseLib/BaseLib/libvideoGo /home/chenzk/BaseLib/BaseLib/libvideoGo /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go /home/chenzk/BaseLib/BaseLib/libvideoGo/build_go/CMakeFiles/media.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/media.dir/depend

