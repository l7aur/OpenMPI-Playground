#include "Grid.hpp"
#include "Parser.hpp"

#include <thread>
#include <chrono>
#include <vector>
#include <barrier>
#include <numeric>
#include <iostream>
#include <atomic>

int main(int argc, char* argv[]) {
    auto [path, max_diff, workers] = Parser::parse(argc, argv);
    
    Grid g;
    g.read(path);

    int grid_size = g.get_grid_size();
    int thread_grid_size = grid_size / workers;
    std::atomic<float> diff = 0.0f;
    std::vector<float> local_diffs(workers);
    std::barrier b(workers);

    auto compute_global_err = [&] {
        return std::accumulate(local_diffs.begin(), local_diffs.end(), 0.0f) / (grid_size * grid_size);
    };

    auto work = [&] (int start_r, int end_r, int worker_id) {
        while (true) {
            local_diffs.at(worker_id) = g.solve(start_r, 0, end_r, grid_size);

            b.arrive_and_wait();

            if (worker_id == 0) {
                diff = compute_global_err();
                g.setup_for_new_iteration();
            }

            b.arrive_and_wait();

            if (diff < max_diff)
                break;
        }
    };

    std::vector<std::thread> threads;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < workers; i++) {
        int start_r = i * thread_grid_size;
        int end_r = (i == workers - 1) ? grid_size : (i + 1) * thread_grid_size;
        threads.emplace_back(std::thread(
            work,
            start_r,
            end_r,
            i
        ));
    }
    for (auto& t : threads)
        if (t.joinable())
            t.join();
    auto finish_time = std::chrono::high_resolution_clock::now();

    std::cout 
        << "Execution time " 
        << std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time - start_time).count() / 1'000'000'000.0 // seconds
        << std::endl;
    // g.print_read_buffer();
    return 0;
}