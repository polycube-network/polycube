add_subdirectory(BaseModel)
add_subdirectory(Lib)

set(data_sources ${lib_sources} ${base_model_sources} PARENT_SCOPE)
