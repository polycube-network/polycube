configure_file(version.go.in ${CMAKE_CURRENT_SOURCE_DIR}//version.go)
install(CODE
"execute_process (
COMMAND /bin/sh -c \"GOPATH=$ENV{GOPATH} USER=$ENV{USER} ${CMAKE_SOURCE_DIR}/src/polycubectl/install.sh\" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/polycubectl/ 
ERROR_VARIABLE _err 
RESULT_VARIABLE _res )

if (NOT \${_res} EQUAL \"0\")                                                                                                                                              
    message( SEND_ERROR \"err: \${_err}, res: \${_res}\")                                                                                                                    
    message( INFO \" Proceeding with any remaining steps, but 'make install' will fail\")                                                                                    
endif ()

")


# this is the right approach but it is not working on ubuntu, for some reason
# the cmake package is not installed in ubuntu

#find_package(bash-completion)
#if(BASH_COMPLETION_FOUND)
#  message(STATUS
#    "Using bash completion dir ${BASH_COMPLETION_COMPLETIONSDIR}")
#else()
#  set (BASH_COMPLETION_COMPLETIONSDIR "/etc/bash_completion.d")
#  message (STATUS
#    "Using fallback bash completion dir ${BASH_COMPLETION_COMPLETIONSDIR}")
#endif()
#
#install(completion/polycubectl.sh polycubectl DESTINATION
#  ${BASH_COMPLETION_COMPLETIONSDIR})

# get the path were to install the completion scripts
# more info (https://github.com/scop/bash-completion)
execute_process (
  COMMAND pkg-config --variable=completionsdir bash-completion
  OUTPUT_VARIABLE COMPLETION_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(
  FILES completion/polycubectl.sh
  DESTINATION ${COMPLETION_PATH}
  RENAME polycubectl
)
