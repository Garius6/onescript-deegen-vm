add_executable(test_loader test_loader.cpp)
target_link_libraries(test_loader PRIVATE onescript_vm_core)
add_test(NAME loader COMMAND test_loader)

add_executable(test_interp test_interp.cpp)
target_link_libraries(test_interp PRIVATE onescript_vm_core)
add_test(NAME interp COMMAND test_interp)
