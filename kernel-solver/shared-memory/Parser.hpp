#pragma once

#include <filesystem>

class Parser {
public:
    struct parsed {
        std::filesystem::path dataset_path;
        float maximum_admissible_err;
        int number_of_workers;
    };
    static parsed parse(
        const int argc,
        char* argv[]
    );
};