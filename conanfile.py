from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

class TrantorConan(ConanFile):
    name = "Trantor"
    version = "0.1" # Don't care. Not publishing this
    
    # Define the options
    options = {
        "tls_provider": ["openssl", "botan", 'none', '']
    }
    
    # Set default option
    default_options = {
        "tls_provider": "openssl"
    }
    
    # Define the dependencies
    requires = (
        "gtest/1.10.0",
        # "c-ares/1.17.1",
        "spdlog/1.12.0"
    )
    
    # Generator for CMake
    generators = "CMakeToolchain"
    
    def configure(self):
        if self.options.tls_provider == "openssl":
            self.requires("openssl/1.1.1t")
        elif self.options.tls_provider == "botan":
            self.requires("botan/3.4.0")
        elif self.options.tls_provider == "none" or self.options.tls_provider == "":
            pass
    
    def build_requirements(self):
        self.build_requires("cmake")  # Example: specify build requirements if needed
