#
# Copyright 2010 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#go and fetch the real compiler outputs because the compiler does things wrong and CMake is too damn brittle to cope
#just incidentally, why the heck does aslink look for a .lst input? why should it care?

#first the .rel
get_filename_component(source_noext ${SOURCE} NAME_WE)
get_filename_component(source_path ${SOURCE} PATH)
set(compiled_ext .rel)
list(APPEND compiled_filepath ${source_path}/${source_noext}${compiled_ext})
#EXECUTE_PROCESS(COMMAND echo Moving ${compiled_filepath} to ${FILE})
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E rename ${compiled_filepath} ${FILE})

#now do the same for the .lst
set(compiled_lst .lst)
get_filename_component(src_ext ${SOURCE} EXT)
get_filename_component(lst_noext ${FILE} NAME_WE)
get_filename_component(lst_path ${FILE} PATH)
list(APPEND compiled_lstpath ${source_path}/${source_noext}${compiled_lst})
list(APPEND compiled_outputlstpath ${lst_path}/${lst_noext}${src_ext}${compiled_lst})
#EXECUTE_PROCESS(COMMAND echo Moving ${compiled_lstpath} to ${compiled_outputlstpath})
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E rename ${compiled_lstpath} ${compiled_outputlstpath})
