from conans import ConanFile, CMake, tools


class AppConan(ConanFile):
    name = "app"
    version = "0.0.1"
    license = "LGPL"
    author = "Kurt SEINITZER <kseinitzer@freifallzeit.at>"
    description = "Test Application package for integration test"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = "liba/0.0.1@ks/test"
    exports_sources = "src*"

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="src")
        cmake.build()


    def package(self):
        self.copy("*.h", dst="include", src="hello")
        self.copy("*hello.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["hello"]

