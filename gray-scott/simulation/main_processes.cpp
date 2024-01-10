#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <adios2.h>
#include <mpi.h>

#include "../../gray-scott/common/timer.hpp"
#include "../../gray-scott/simulation/gray-scott.h"
#include "../../gray-scott/simulation/restart.h"
#include "../../gray-scott/simulation/writer.h"

void print_io_settings(const adios2::IO &io)
{
    std::cout << "Simulation writes data using engine type:              "
              << io.EngineType() << std::endl;
    auto ioparams = io.Parameters();
    std::cout << "IO parameters:  " << std::endl;
    for (const auto &p : ioparams)
    {
        std::cout << "    " << p.first << " = " << p.second << std::endl;
    }
}

void print_settings(const Settings &s, int restart_step)
{
    std::cout << "grid:             " << s.L << "x" << s.L << "x" << s.L
              << std::endl;
    if (restart_step > 0)
    {
        std::cout << "restart:          from step " << restart_step
                  << std::endl;
    }
    else
    {
        std::cout << "restart:          no" << std::endl;
    }
    std::cout << "steps:            " << s.steps << std::endl;
    std::cout << "plotgap:          " << s.plotgap << std::endl;
    std::cout << "F:                " << s.F << std::endl;
    std::cout << "k:                " << s.k << std::endl;
    std::cout << "dt:               " << s.dt << std::endl;
    std::cout << "Du:               " << s.Du << std::endl;
    std::cout << "Dv:               " << s.Dv << std::endl;
    std::cout << "noise:            " << s.noise << std::endl;
    std::cout << "output:           " << s.output << std::endl;
    std::cout << "adios_config:     " << s.adios_config << std::endl;
}

void print_simulator_settings(const GrayScott &s)
{
    std::cout << "process layout:   " << s.npx << "x" << s.npy << "x" << s.npz
              << std::endl;
    std::cout << "local grid size:  " << s.size_x << "x" << s.size_y << "x"
              << s.size_z << std::endl;
}

int main(int argc, char **argv)
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    const unsigned int color = 1;
    MPI_Comm comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    if (argc < 2)
    {
        if (rank == 0)
        {
            std::cerr << "Too few arguments" << std::endl;
            std::cerr << "Usage: gray-scott settings.json" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    Settings settings = Settings::from_json(argv[1]);

    GrayScott sim(settings, comm);
    sim.init();

    adios2::ADIOS adios(settings.adios_config, comm);
    adios2::IO io_main = adios.DeclareIO("SimulationOutput");
    adios2::IO io_ckpt = adios.DeclareIO("SimulationCheckpoint");

    int restart_step = 0;
    if (settings.restart)
    {
        restart_step = ReadRestart(comm, settings, sim, io_ckpt);
        io_main.SetParameter("AppendAfterSteps",
                             std::to_string(restart_step / settings.plotgap));
    }

    Writer writer_main(settings, sim, io_main);
    writer_main.open(settings.output, (restart_step > 0));

    if (rank == 0)
    {
        print_io_settings(io_main);
        std::cout << "========================================" << std::endl;
        print_settings(settings, restart_step);
        print_simulator_settings(sim);
        std::cout << "========================================" << std::endl;
    }


    double start_t_time, end_t_time, elapsed_t_time;
    double start_c_time, end_c_time, elapsed_c_time;
    double start_w_time, end_w_time, elapsed_w_time;
    for (int it = restart_step; it < settings.steps;)
    {
        // TEST TIME
        MPI_Barrier(comm);
        start_t_time = MPI_Wtime();
        start_c_time = MPI_Wtime();

        sim.iterate();
        it++;
        // TEST TIME
        end_c_time = MPI_Wtime();
        elapsed_c_time = end_c_time - start_c_time;
        MPI_Barrier(comm);


        if (it % settings.plotgap == 0)
        {
            // TEST TIME
            MPI_Barrier(comm);
            start_w_time = MPI_Wtime();

            writer_main.write(it, sim);
            if (rank == 0)
            {
                std::cout << "Simulation at step " << it
                          << " writing output step     "
                          << it / settings.plotgap << std::endl;
            }
            // TEST TIME
            end_w_time = MPI_Wtime();
            elapsed_w_time = end_w_time - start_w_time;
            end_t_time = MPI_Wtime();
            elapsed_t_time = end_t_time - start_t_time;
            MPI_Barrier(comm);

        }

        if (settings.checkpoint && (it % settings.checkpoint_freq) == 0)
        {
            WriteCkpt(comm, it, settings, sim, io_ckpt);
        }

        // Gather all elapsed times from different processes
        double *all_elapsed_t_times = NULL;
        double *all_elapsed_c_times = NULL;
        double *all_elapsed_w_times = NULL;
        if (rank == 0) {
            all_elapsed_t_times = (double *)malloc(procs * sizeof(double));
            all_elapsed_c_times = (double *)malloc(procs * sizeof(double));
            all_elapsed_w_times = (double *)malloc(procs * sizeof(double));
        }
        MPI_Gather(&elapsed_t_time, 1, MPI_DOUBLE, all_elapsed_t_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&elapsed_c_time, 1, MPI_DOUBLE, all_elapsed_c_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&elapsed_w_time, 1, MPI_DOUBLE, all_elapsed_w_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Print elapsed time for each process (rank 0 prints all times)
        if (rank == 0) {
            printf("Elapsed times for each process to :\n");
            printf("It is in step %d, by process %d\n", it, rank);
            for (int i = 0; i < procs; ++i) {
                printf("(Compute)Process %d: %f seconds\n", i, all_elapsed_c_times[i]);
                if (it % settings.plotgap == 0){
                    printf("(Write)Process %d: %f seconds\n", i, all_elapsed_w_times[i]);
                    printf("(Total)Process %d: %f seconds\n", i, all_elapsed_t_times[i]);
                }
            }
            free(all_elapsed_c_times);
            free(all_elapsed_w_times);
            free(all_elapsed_t_times);
        }

        
    }

    writer_main.close();



    MPI_Finalize();
}
