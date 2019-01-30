find_package(nlohmann_json 3.5.0 REQUIRED)

add_subdirectory(Body)
add_subdirectory(Endpoint)
add_subdirectory(Data)
set(resources_sources
        ${body_sources}
        ${endpoint_sources}
        ${data_sources}
        PARENT_SCOPE)