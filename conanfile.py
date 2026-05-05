from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout

class GmcX264(ConanFile):
    name        = "gmc_x264"
    version     = "0.1.0"
    description = "GMC preprocessor — LD_PRELOAD wrapper dla libx264"
    license     = "MIT"
    settings    = "os", "compiler", "build_type", "arch"

    requires = [
        "opencv/4.9.0",
        "libx264/cci.20220602",
    ]

    test_requires = [
    "gtest/1.14.0",
    ]


    options = {
        "shared":        [True, False],
        "grass_refresh": [5, 10, 15, 30],
        "debug_logging": [True, False],
    }

    default_options = {
        "shared":        True,
        "grass_refresh": 15,
        "debug_logging": False,
        # GUI — wyłącz całkowicie, wrapper nie wyświetla okien
        "opencv/*:with_ffmpeg":  False,
        "opencv/*:with_qt":      False,
        "opencv/*:dnn":          False,
        "opencv/*:with_cuda":    False,
        "opencv/*:with_gtk":     False,
        "opencv/*:highgui":      False,   # ← dodaj to
        "opencv/*:with_wayland": False,   # ← i to
        "opencv/*:with_opengl":  False,   # ← i to
    }

    def layout(self):
        cmake_layout(self)

    def generate(self):
        # CMakeDeps — generuje Find*.cmake dla każdej zależności
        deps = CMakeDeps(self)
        deps.generate()

        # CMakeToolchain — generuje CMakePresets.json z ustawieniami kompilatora
        tc = CMakeToolchain(self)

        # Przekaż opcje Conan jako defines do C++
        tc.variables["GRASS_REFRESH_FRAMES"] = int(self.options.grass_refresh)
        tc.variables["DEBUG_LOGGING"]        = bool(self.options.debug_logging)
        tc.variables["WITH_TESTS"] = True

        tc.generate()

    def build(self):
        from conan.tools.cmake import CMake
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        from conan.tools.cmake import CMake
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["gmc_x264"]
