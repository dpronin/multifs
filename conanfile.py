# how to use semver https://devhints.io/semver

from conans import ConanFile, CMake, tools

class multifs(ConanFile):
    name = "multifs"
    author = "Denis Pronin"
    description = "Multi File System"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"

    scm = {
        "type": "git",
        "subfolder": name,
        "url": "auto",
        "revision": "auto",
        "username": "git"
    }

    def requirements(self):
        self.requires("libfuse/3.10.5")
