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


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

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
CMAKE_COMMAND = /usr/local/bin/cmake-3.16.6-Linux-x86_64/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake-3.16.6-Linux-x86_64/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zhangzhilong/High_Performance_Server/epoll_unblock

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zhangzhilong/High_Performance_Server/epoll_unblock

# Include any dependencies generated for this target.
include CMakeFiles/High_Performance_Server.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/High_Performance_Server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/High_Performance_Server.dir/flags.make

CMakeFiles/High_Performance_Server.dir/server.cpp.o: CMakeFiles/High_Performance_Server.dir/flags.make
CMakeFiles/High_Performance_Server.dir/server.cpp.o: server.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhangzhilong/High_Performance_Server/epoll_unblock/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/High_Performance_Server.dir/server.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/High_Performance_Server.dir/server.cpp.o -c /home/zhangzhilong/High_Performance_Server/epoll_unblock/server.cpp

CMakeFiles/High_Performance_Server.dir/server.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/High_Performance_Server.dir/server.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhangzhilong/High_Performance_Server/epoll_unblock/server.cpp > CMakeFiles/High_Performance_Server.dir/server.cpp.i

CMakeFiles/High_Performance_Server.dir/server.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/High_Performance_Server.dir/server.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhangzhilong/High_Performance_Server/epoll_unblock/server.cpp -o CMakeFiles/High_Performance_Server.dir/server.cpp.s

CMakeFiles/High_Performance_Server.dir/wrap.cpp.o: CMakeFiles/High_Performance_Server.dir/flags.make
CMakeFiles/High_Performance_Server.dir/wrap.cpp.o: wrap.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhangzhilong/High_Performance_Server/epoll_unblock/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/High_Performance_Server.dir/wrap.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/High_Performance_Server.dir/wrap.cpp.o -c /home/zhangzhilong/High_Performance_Server/epoll_unblock/wrap.cpp

CMakeFiles/High_Performance_Server.dir/wrap.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/High_Performance_Server.dir/wrap.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhangzhilong/High_Performance_Server/epoll_unblock/wrap.cpp > CMakeFiles/High_Performance_Server.dir/wrap.cpp.i

CMakeFiles/High_Performance_Server.dir/wrap.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/High_Performance_Server.dir/wrap.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhangzhilong/High_Performance_Server/epoll_unblock/wrap.cpp -o CMakeFiles/High_Performance_Server.dir/wrap.cpp.s

# Object files for target High_Performance_Server
High_Performance_Server_OBJECTS = \
"CMakeFiles/High_Performance_Server.dir/server.cpp.o" \
"CMakeFiles/High_Performance_Server.dir/wrap.cpp.o"

# External object files for target High_Performance_Server
High_Performance_Server_EXTERNAL_OBJECTS =

High_Performance_Server: CMakeFiles/High_Performance_Server.dir/server.cpp.o
High_Performance_Server: CMakeFiles/High_Performance_Server.dir/wrap.cpp.o
High_Performance_Server: CMakeFiles/High_Performance_Server.dir/build.make
High_Performance_Server: CMakeFiles/High_Performance_Server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zhangzhilong/High_Performance_Server/epoll_unblock/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable High_Performance_Server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/High_Performance_Server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/High_Performance_Server.dir/build: High_Performance_Server

.PHONY : CMakeFiles/High_Performance_Server.dir/build

CMakeFiles/High_Performance_Server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/High_Performance_Server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/High_Performance_Server.dir/clean

CMakeFiles/High_Performance_Server.dir/depend:
	cd /home/zhangzhilong/High_Performance_Server/epoll_unblock && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zhangzhilong/High_Performance_Server/epoll_unblock /home/zhangzhilong/High_Performance_Server/epoll_unblock /home/zhangzhilong/High_Performance_Server/epoll_unblock /home/zhangzhilong/High_Performance_Server/epoll_unblock /home/zhangzhilong/High_Performance_Server/epoll_unblock/CMakeFiles/High_Performance_Server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/High_Performance_Server.dir/depend
