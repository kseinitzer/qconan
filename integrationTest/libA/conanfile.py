from conans import ConanFile, CMake, tools


class LibaConan(ConanFile):
    name = "liba"
    version = "0.0.1"
    license = "LGPL"
    author = "Kurt SEINITZER kseinitzer@freifallzeit.at"
    description = "Test package for integration test"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    exports_sources = "src*"
    generators = "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="src")
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="src")
        self.copy("*libb.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["liba"]

