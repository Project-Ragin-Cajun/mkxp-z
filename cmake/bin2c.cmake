# Converts a binary file into a C byte array with the same naming
# convention xxd uses (path components become underscores).
#
# Usage:
#   cmake -DINPUT=../shader/common.h -DOUTPUT=xxd/common.h.xxd -P cmake/bin2c.cmake

if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "Usage: cmake -DINPUT=<file> -DOUTPUT=<file> -P bin2c.cmake")
endif()

# Read the file as hex
file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" hex_len)
math(EXPR byte_count "${hex_len} / 2")

# Build C identifier from input path (same as xxd: non-alnum -> underscore)
string(REGEX REPLACE "[^a-zA-Z0-9]" "_" identifier "${INPUT}")

# Bulk convert: insert "0x" before each byte pair and ", " after
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " formatted "${hex}")

# Insert newlines every 12 bytes (24 hex chars -> 72 formatted chars per line)
# Each "0xNN, " is 6 chars, so 12 per line = 72 chars
string(REGEX REPLACE "(0x[0-9a-f][0-9a-f], ){12}" "  \\0\n" formatted "${formatted}")

# Indent lines that didn't get the leading spaces from the above
string(REGEX REPLACE "\n(0x)" "\n  \\1" formatted "${formatted}")

# Add leading indent to first line
set(formatted "  ${formatted}")

# Write the output
file(WRITE "${OUTPUT}"
        "unsigned char ${identifier}[] = {\n${formatted}\n};\nunsigned int ${identifier}_len = ${byte_count};\n"
)