# Loads the contents of a file into a std::string variable
#
# It creates a header file in ${CMAKE_CURRENT_BINARY_DIR}/${file}.h
# that wrapps the contents of the file in a std::string using the raw
# string literal feature of C++11. The user needs to include that file
# into the source code in order to see the variable.
#
# parameters:
#  target: target to add a dependency on file
#  file: file to be loaded
#  variable_name: name variable where the file is loaded
#
# example:
#  load_file_as_variable(my-lib resource.c my_resource)
# Creates a resource.h in CMAKE_CURRENT_BINARY_DIR with a string variable
# my_resource with the contents of resource.c
# A dependency in resource.c is added to my-lib

function(load_file_as_variable target file variable_name)
    get_filename_component(file_name ${file} NAME_WE)
    get_filename_component(file_dir ${file} DIRECTORY)

    set(new_path ${file_dir}/${file_name}.h)

    add_custom_command(
            OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/${new_path}
            COMMAND
            mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${file_dir}
            COMMAND
            echo "#pragma once" > ${CMAKE_CURRENT_BINARY_DIR}/${new_path}
            COMMAND
            echo "#include <string>" >> ${CMAKE_CURRENT_BINARY_DIR}/${new_path}
            COMMAND
            echo "const std::string ${variable_name} = R\"POLYCUBE_DP(" >> ${CMAKE_CURRENT_BINARY_DIR}/${new_path}
            COMMAND
            cat ${CMAKE_CURRENT_SOURCE_DIR}/${file} >> ${CMAKE_CURRENT_BINARY_DIR}/${new_path}
            COMMAND
            cmake -E echo ")POLYCUBE_DP\";" >> ${CMAKE_CURRENT_BINARY_DIR}/${new_path}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file}
            VERBATIM
    )

    string(REPLACE "/" "-" path_replaced ${new_path})

    add_custom_target(
            generate_${path_replaced}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${new_path}
    )

    add_dependencies(${target} generate_${path_replaced})
endfunction()
