#include "Parser.hpp"

Parser::parsed Parser::parse(
    const int argc,
    char* argv[]
)
{
    parsed p;

    if (argc != 3)
        throw std::runtime_error("Usage: ./main <path> <max_err>");

    p.dataset_path = std::filesystem::path(argv[1]);
    p.maximum_admissible_err = std::stof(argv[2]); 

    if (p.maximum_admissible_err < 0.0f)
        throw std::runtime_error("<max_error> falls outside the predefined boundaries [0.0; ...]");

    if (!std::filesystem::exists(p.dataset_path) || !std::filesystem::is_regular_file(p.dataset_path))
        throw std::runtime_error(p.dataset_path.string() + " is not accessible/a valid file");

    return p;
}
