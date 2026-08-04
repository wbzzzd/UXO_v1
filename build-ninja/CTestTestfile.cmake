# CMake generated Testfile for 
# Source directory: /home/lin/UXO_v1-detection
# Build directory: /home/lin/UXO_v1-detection/build-ninja
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(startup_visible "/home/lin/UXO_v1-detection/build-ninja/UXOMissionControlStartupTest")
set_tests_properties(startup_visible PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "/home/lin/UXO_v1-detection/CMakeLists.txt;55;add_test;/home/lin/UXO_v1-detection/CMakeLists.txt;0;")
add_test(demo_scenario_provider "/home/lin/UXO_v1-detection/build-ninja/UXOMissionControlDemoScenarioTest")
set_tests_properties(demo_scenario_provider PROPERTIES  _BACKTRACE_TRIPLES "/home/lin/UXO_v1-detection/CMakeLists.txt;68;add_test;/home/lin/UXO_v1-detection/CMakeLists.txt;0;")
add_test(simulation_workflow "/home/lin/UXO_v1-detection/build-ninja/UXOMissionControlSimulationWorkflowTest")
set_tests_properties(simulation_workflow PROPERTIES  _BACKTRACE_TRIPLES "/home/lin/UXO_v1-detection/CMakeLists.txt;81;add_test;/home/lin/UXO_v1-detection/CMakeLists.txt;0;")
add_test(simulation_workflow_ui "/home/lin/UXO_v1-detection/build-ninja/UXOMissionControlSimulationWorkflowUiTest")
set_tests_properties(simulation_workflow_ui PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_OPENGL=software;LIBGL_ALWAYS_SOFTWARE=1" _BACKTRACE_TRIPLES "/home/lin/UXO_v1-detection/CMakeLists.txt;97;add_test;/home/lin/UXO_v1-detection/CMakeLists.txt;0;")
subdirs("src/Common")
subdirs("src/Core")
subdirs("src/MainWindow")
subdirs("src/App")
