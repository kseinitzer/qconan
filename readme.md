# qconan

Provide conan support for QtCreator. 



## Introduction

Working with conan solves a lot of issues but brings additional overhead when it comes to IDE topics. In the case of QtCreator a typical work-flow will look like: 

1. create a build folder and change into the folder
2. call conan install .. -g virtualenv to create the virtual environment files needed
3. call the environment file: source activate.sh
4. call build to create CMake specific files in the build folder (cache file)
5. open QtCreator from this shell to use the modified environment and allow the creator to auto-detect the correct compilers used
6. open the project and configure it by calling "import existing build"

If the conan file changes there is a good chance to repeat some or all of those steps. Especially if the developer needs to work with multiple different projects the handling can become cumbersome. 

The idea is to simplify this process as much as possible. The abbreviation IDE stands for Integrated Development Environment and therefor also the conan handling shall be fully integrated and not hacked around our IDE. 

## Qt Creator

All configuration settings in a kit are realized by KitAspect implementations. A specially the EnvironmentKitAspect is interesting for the "conan kit" feature.



The existing infrastructure should be reused as efficient as possible. For example  OsSpecificAspects::pathListSeparator(OsType) and HostOsInfo::hostOs()

## Install

  - Close all open instances of QtCreator.
  - Download the plugin zip that matches your architecture and QtCreator version and extract the plugin.
  - Place the plugin in your QtCreator installation directory in <QtInstallDir>/lib/qtcreator/plugins.
  - Start QtCreator and check if the plugin is listed in the Plugin dialog. Menubar>Help/About Plugins. The conan plugin should be installed now. 

## Help

Configuration options can be provided by the qconan.ini file placed beside the project file. The following settings can be taken:

 - global/path
 - global/installFlags
 - environment/useLibPath
 - environment/useBinPath

All fields are optional.

### global/path

If the path key is given, the value is interpreted as the path to the conanfile.py. If the key is not used the plugin tries to find a conanfile.py on the same level as the root project and one level above. 

### global/installFlags

If the installFlags key is given the value will be added to every conan install command.

### environment/useLibPath

Add the library path of each dependency to the PATH run-environment variable in the project options. This can be used on a windows system to provide the OS the needed information, to find referenced DLL's from the package dependency tree.

### environment/useBinPath

Add the binary path of each dependency to the PATH run-environment variable in the project options. This can be used on a windows system to provide the OS the needed information, to find referenced DLL's from the package dependency tree.

## Features

List of implemented features.

### 0.2.0

#### use package PATH information for run-settings

The PATH information provided by conan will be used to parametrize the run-settings of the active project.

#### linux support

Added support for following QtCreator versions:

  - 4.7.2 x64
  - 4.8.2 x64
  - 4.10.1 x64
  - 4.11.2 x64
  - 4.12.0 x64

#### mac support

Added support for following QtCreator versions:

  - 4.7.2
  - 4.8.2
  - 4.10.2
  - 4.11.2
  - 4.12.0

#### windows support

Added support for following QtCreator versions:

  - 4.8.2 VS2015 32-bit
  - 4.10.1 VS2017 32/64-bit
  - 4.11.2 VS2017 32/64-bit
  - 4.12.0 VS2017 32/64-bit

### 0.1.0

#### detect change of conanfile.py

In case the conanfile.py is updated, the conan plugin will reinstall the package so that the changes will take place immediately. 

#### use >General Messages< tab for diagnostic information

Information from the conan plugin is displayed in the **General Messages** tab. Messages from the conan plugin begin are prefixed by **conan plugin:**.

#### custom conan installation flags

Allow special needed flags to be added to the automatically executed install command. The flags can be defined by a project-specific configuration file. See section help for more information.

#### automatic conan install if build folder change

In case the build directory is changed the conan install command will be triggered automatically.

#### automatic conan install if target change

In case the build target is changed (e.g. from debug to release) the conan install command will be triggered automatically.

