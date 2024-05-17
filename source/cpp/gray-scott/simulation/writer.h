#ifndef __WRITER_H__
#define __WRITER_H__

#include <adios2.h>
#include <mpi.h>

#include "../../gray-scott/simulation/gray-scott.h"
#include "../../gray-scott/simulation/settings.h"

class Writer
{
public:
    Writer(const Settings &settings, const GrayScott &sim);
    void Wopen(const std::string &fname);
    void fast_write(void *buf, size_t size);
    void Wwrite(int step, const GrayScott &sim, MPI_Comm comm, int rank);
    void Wclose();

protected:
    //write mode
    int fd;
    //exscan, for each step
    size_t perrank, writen_thisprocessor;
    //allreduce, for steps
    size_t perstep=0;
    size_t writen_thisstep;

    //DUMB
    size_t increase,writenumber=0,writenumberstep,nextnumber=0,plus;

    Settings settings;

};

#endif