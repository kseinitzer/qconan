# qconan

Provide conan support for QtCreator. 



## Introduction

Basically working with conan solves a lot of issues but brings additional overhead when it comes to IDE topics. In the case of QtCreator a typical work-flow will look like: 

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

## Help

Configuration options can be provided by the qconan.ini file placed beside the project file. The following settings can be taken:

 - global/path
 - global/installFlags

All fields are optional. If the path key is given, the value is interpreted as the path to the conanfile.py. If the key is not used the plugin tries to find a conanfile.py on the same level as the root project and one level above. If the installFlags key is given the value will be added to every conan install command.

## Features

This section lists some top-level ideas on how to simplify the conan usage on a feature level. 

### dependency tree

A graphical dependency tree to visualize the package dependency including all override and package options.

### conan kit

A special build kit directly derived from a conan file. The environment variables from the conan file will be used to parametrize the kit's environment. This way QtCreator can be started without any special environment. 

The kit must be set up with the correct toolchain values. KitInformation.cpp - ToolChainKitAspect may help.

### dependency generator

A custom conan generator that creates an easy parseable file. 

### automatic conanfile detection

In case a new project is loaded the conan plugin will search for a conanfile.py and ask the user if the conan plugin should take care of this project. This setting shall be changeable in the project related configuration widget.

### conan profile

Sometimes it is necessary to work on the same package with different profiles. A conan profile is similar to the kit approach from QtCreator. This feature should allow linking one conan profile to one QtCreator kit. 
