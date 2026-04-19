find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(spellmaster_ui spellmaster_ui.cpp)
target_link_libraries(spellmaster_ui
	Qt6::Widgets
	nlohmann_json::nlohmann_json
)
