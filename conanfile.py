from conans import ConanFile, CMake, tools


class SimperiumcConan(ConanFile):
    name = "simperium-c"
    version = "0.1.0"
    license = "MIT"
    url = "https://github.com/franc0is/simperium-c"
    description = "A C library to integrate Simperium's service"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    requires = (("jansson/2.11@franc0is/stable"),
               ("jsondiff-c/0.1.0@franc0is/stable"),
               ("argtable3/3.0.3@franc0is/stable"),
               ("libcurl/7.56.1@bincrafters/stable"),
               ("libwebsockets/2.4.0@bincrafters/stable"),
               ("doctest/1.2.6@bincrafters/stable"))

    exports_sources = "*"

    def configure(self):
        self.settings.compiler = 'gcc'
        self.settings.compiler.version = 7
        self.settings.compiler.libcxx = 'libstdc++11'

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder=".")
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src=".")
        self.copy("*simperium.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["simperium"]
