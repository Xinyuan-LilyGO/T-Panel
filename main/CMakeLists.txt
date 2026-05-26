if(CONFIG_EXAMPLE_BUILD_SCREEN)
    set(project_path "examples/screen/")
endif()

idf_component_register(
    SRC_DIRS ${project_path}
    INCLUDE_DIRS ${project_path}
)
