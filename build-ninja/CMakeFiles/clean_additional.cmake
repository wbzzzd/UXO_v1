# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/UXOMissionControlDemoScenarioTest_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/UXOMissionControlDemoScenarioTest_autogen.dir/ParseCache.txt"
  "CMakeFiles/UXOMissionControlSimulationWorkflowTest_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/UXOMissionControlSimulationWorkflowTest_autogen.dir/ParseCache.txt"
  "CMakeFiles/UXOMissionControlSimulationWorkflowUiTest_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/UXOMissionControlSimulationWorkflowUiTest_autogen.dir/ParseCache.txt"
  "CMakeFiles/UXOMissionControlStartupTest_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/UXOMissionControlStartupTest_autogen.dir/ParseCache.txt"
  "UXOMissionControlDemoScenarioTest_autogen"
  "UXOMissionControlSimulationWorkflowTest_autogen"
  "UXOMissionControlSimulationWorkflowUiTest_autogen"
  "UXOMissionControlStartupTest_autogen"
  "src/App/CMakeFiles/UXOMissionControl_autogen.dir/AutogenUsed.txt"
  "src/App/CMakeFiles/UXOMissionControl_autogen.dir/ParseCache.txt"
  "src/App/UXOMissionControl_autogen"
  "src/Common/CMakeFiles/Common_autogen.dir/AutogenUsed.txt"
  "src/Common/CMakeFiles/Common_autogen.dir/ParseCache.txt"
  "src/Common/Common_autogen"
  "src/Core/CMakeFiles/Core_autogen.dir/AutogenUsed.txt"
  "src/Core/CMakeFiles/Core_autogen.dir/ParseCache.txt"
  "src/Core/Core_autogen"
  "src/MainWindow/CMakeFiles/MainWindow_autogen.dir/AutogenUsed.txt"
  "src/MainWindow/CMakeFiles/MainWindow_autogen.dir/ParseCache.txt"
  "src/MainWindow/MainWindow_autogen"
  )
endif()
