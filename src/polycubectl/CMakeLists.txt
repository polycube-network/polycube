configure_file(version.go.in ${CMAKE_CURRENT_SOURCE_DIR}//version.go)
install(CODE
"execute_process( COMMAND /bin/sh -c \"GOPATH=$ENV{GOPATH} USER=$ENV{USER} ${CMAKE_SOURCE_DIR}/src/polycubectl/install.sh\"
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/polycubectl/ )")
