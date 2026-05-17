#include "Parser.hpp"
#include "Worker.hpp"
#include "MPIContext.hpp"

int main(int argc, char* argv[])
{
    auto [path, max_diff] = Parser::parse(argc, argv);

    auto context = MPIContext();

    auto worker = Worker(
        context.get_cartesian_comm(),
        context.get_rank(),
        context.get_position(), 
        path, 
        max_diff, 
        context.get_grid_size()
    );
    worker.solve(context.get_world_size(), max_diff);

    return 0;
}