# Global settings

cmake_minimum_required( VERSION 2.8 )
cmake_policy(SET CMP0048 NEW)


# Define the project
project( Hyperion VERSION 4.0 LANGUAGES C )


# Add SoftFloat subdirectory
add_subdirectory(SoftFloat)

# Add crypto subdirectory
add_subdirectory(crypto)

# Add decNumber subdirectory
add_subdirectory(decNumber)

# Add telnet subdirectory
add_subdirectory(telnet)
