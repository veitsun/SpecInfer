#------------------------------------------------------------------------------#
# Copyright 2022 Stanford University
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.1)
project(LegionTest_ctrl_repl_safety)

# Only search if were building stand-alone and not as part of Legion
if(NOT Legion_SOURCE_DIR)
  find_package(Legion REQUIRED)
endif()

add_executable(ctrl_repl_safety ctrl_repl_safety.cc)
target_link_libraries(ctrl_repl_safety Legion::Legion)
if(Legion_ENABLE_TESTING)
  add_test(NAME ctrl_repl_safety COMMAND ${Legion_TEST_LAUNCHER} $<TARGET_FILE:ctrl_repl_safety> ${Legion_TEST_ARGS})
endif()
