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
CMAKE_SOURCE_DIR = /home/sloan/桌面/udp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sloan/桌面/udp

# Include any dependencies generated for this target.
include CMakeFiles/ModifyData.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ModifyData.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ModifyData.dir/flags.make

addressbook.pb.h: addressbook.proto
addressbook.pb.h: /usr/local/bin/protoc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/sloan/桌面/udp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Running cpp protocol buffer compiler on addressbook.proto"
	/usr/local/bin/protoc --cpp_out /home/sloan/桌面/udp -I /home/sloan/桌面/udp /home/sloan/桌面/udp/addressbook.proto

addressbook.pb.cc: addressbook.pb.h
	@$(CMAKE_COMMAND) -E touch_nocreate addressbook.pb.cc

CMakeFiles/ModifyData.dir/ModifyData.cpp.o: CMakeFiles/ModifyData.dir/flags.make
CMakeFiles/ModifyData.dir/ModifyData.cpp.o: ModifyData.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sloan/桌面/udp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/ModifyData.dir/ModifyData.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ModifyData.dir/ModifyData.cpp.o -c /home/sloan/桌面/udp/ModifyData.cpp

CMakeFiles/ModifyData.dir/ModifyData.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ModifyData.dir/ModifyData.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sloan/桌面/udp/ModifyData.cpp > CMakeFiles/ModifyData.dir/ModifyData.cpp.i

CMakeFiles/ModifyData.dir/ModifyData.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ModifyData.dir/ModifyData.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sloan/桌面/udp/ModifyData.cpp -o CMakeFiles/ModifyData.dir/ModifyData.cpp.s

CMakeFiles/ModifyData.dir/addressbook.pb.cc.o: CMakeFiles/ModifyData.dir/flags.make
CMakeFiles/ModifyData.dir/addressbook.pb.cc.o: addressbook.pb.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sloan/桌面/udp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/ModifyData.dir/addressbook.pb.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ModifyData.dir/addressbook.pb.cc.o -c /home/sloan/桌面/udp/addressbook.pb.cc

CMakeFiles/ModifyData.dir/addressbook.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ModifyData.dir/addressbook.pb.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sloan/桌面/udp/addressbook.pb.cc > CMakeFiles/ModifyData.dir/addressbook.pb.cc.i

CMakeFiles/ModifyData.dir/addressbook.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ModifyData.dir/addressbook.pb.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sloan/桌面/udp/addressbook.pb.cc -o CMakeFiles/ModifyData.dir/addressbook.pb.cc.s

# Object files for target ModifyData
ModifyData_OBJECTS = \
"CMakeFiles/ModifyData.dir/ModifyData.cpp.o" \
"CMakeFiles/ModifyData.dir/addressbook.pb.cc.o"

# External object files for target ModifyData
ModifyData_EXTERNAL_OBJECTS =

ModifyData: CMakeFiles/ModifyData.dir/ModifyData.cpp.o
ModifyData: CMakeFiles/ModifyData.dir/addressbook.pb.cc.o
ModifyData: CMakeFiles/ModifyData.dir/build.make
ModifyData: /usr/local/lib/libprotobuf.so
ModifyData: CMakeFiles/ModifyData.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/sloan/桌面/udp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX executable ModifyData"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ModifyData.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ModifyData.dir/build: ModifyData

.PHONY : CMakeFiles/ModifyData.dir/build

CMakeFiles/ModifyData.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ModifyData.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ModifyData.dir/clean

CMakeFiles/ModifyData.dir/depend: addressbook.pb.h
CMakeFiles/ModifyData.dir/depend: addressbook.pb.cc
	cd /home/sloan/桌面/udp && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sloan/桌面/udp /home/sloan/桌面/udp /home/sloan/桌面/udp /home/sloan/桌面/udp /home/sloan/桌面/udp/CMakeFiles/ModifyData.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ModifyData.dir/depend

