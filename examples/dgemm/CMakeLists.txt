#
#  This file is a part of TiledArray.
#  Copyright (C) 2013  Virginia Tech
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#  Justus Calvin
#  Department of Chemistry, Virginia Tech
#
#  CMakeLists.txt
#  Sep 4, 2013
#

# Create example executable

foreach(_exec blas eigen ta_band ta_dense ta_sparse ta_dense_nonuniform
              ta_asymm_dense ta_sparse_grow ta_dense_new_tile)

  # Add executable
  add_executable(${_exec} EXCLUDE_FROM_ALL ${_exec}.cpp)
  target_link_libraries(${_exec} PRIVATE tiledarray)
  add_dependencies(${_exec} External)
  add_dependencies(example ${_exec})

endforeach()

if(NOT COREBLAS_DIR)
  SET(COREBLAS_DIR "$ENV{COREBLAS_DIR}")
endif()	   
if(NOT PLASMA_DIR)
  SET(PLASMA_DIR "$ENV{PLASMA_DIR}")
endif()	   
find_package(PARSEC COMPONENTS HWLOC MPI PAPI CUDA)
find_package(COREBLAS)
find_package(PLASMA)
if( PARSEC_FOUND AND COREBLAS_FOUND AND PLASMA_FOUND ) 
include_directories(".;${PARSEC_INCLUDE_DIRS_DEP};${COREBLAS_INCLUDE_DIRS};${PLASMA_INCLUDE_DIRS}")
link_directories("${COREBLAS_LIBRARY_DIRS};${PLASMA_LIBRARY_DIRS};${PARSEC_LIBRARY_DIRS}")
add_executable(ta_cc_abcd EXCLUDE_FROM_ALL ta_cc_abcd.cpp irregular_tiled_matrix_wrapper.h parsec_wrapper.h irregular_tiled_matrix_wrapper.cpp parsec_summa_wrapper.h)
target_link_libraries(ta_cc_abcd PRIVATE tiledarray)
target_link_libraries(ta_cc_abcd PRIVATE ${PARSEC_LIBRARIES_DEP})
target_link_libraries(ta_cc_abcd PRIVATE ${COREBLAS_LIBRARIES})
target_link_libraries(ta_cc_abcd PRIVATE ${PLASMA_LIBRARIES})
add_dependencies(ta_cc_abcd External)
add_dependencies(example ta_cc_abcd)
endif()

if(ENABLE_ELEMENTAL)
    # Add the ta_elem_dense executable
    add_executable(ta_elem_dense EXCLUDE_FROM_ALL ta_elem_dense.cpp)
    target_link_libraries(ta_elem_dense PRIVATE tiledarray)
    add_dependencies(ta_elem_dense External)
    add_dependencies(example ta_elem_dense)
endif()
