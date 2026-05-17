#include "MPIContext.hpp"

MPIContext::MPIContext()
{
    init();
}

void MPIContext::init()
{
    if (MPI_Init(NULL, NULL) != MPI_SUCCESS)
        throw std::runtime_error("Failed to initialize MPI");

    int size = 0;
    if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS)
        throw std::runtime_error("Failed to retrieve world size");

    if (MPI_Dims_create(size, 2, dims) != MPI_SUCCESS)
        throw std::runtime_error("Failed to create grid");

    if (MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cartesian_comm) != MPI_SUCCESS)
        throw std::runtime_error("Failed to initialize grid communicator");

    if (MPI_Comm_rank(cartesian_comm, &rank) != MPI_SUCCESS)
        throw std::runtime_error("Failed to retrieve rank");

    int final_dims[2];
    if (MPI_Cart_get(cartesian_comm, 2, final_dims, periods, position) != MPI_SUCCESS)
        throw std::runtime_error("Failed to retrieve grid dimensions");
    
    grid_size.first = final_dims[0];
    grid_size.second = final_dims[1];
}

MPIContext::~MPIContext()
{
    int flag = 0;
    (void)MPI_Initialized(&flag);
    if (flag)
        (void)MPI_Finalize();
}