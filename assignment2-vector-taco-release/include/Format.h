#pragma once

#include <map>
#include <string>

enum class Format {
    // This assignment only needs to handle Dense and Compressed level formats.
    Dense,
    Compressed,
};

// Maps vector name to level formats.
typedef std::map<std::string, std::vector<Format>> FormatMap;
