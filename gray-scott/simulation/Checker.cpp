#include "/uhome/a01431/ADIOS2/ADIOS2-Examples/source/cpp/gray-scott/simulation/settings.cpp"
#include "/uhome/a01431/ADIOS2/ADIOS2-Examples/source/cpp/gray-scott/simulation/gray-scott.h"

#include <stdio.h>
#include <string.h>

int main () // Compile: /opt/nec/ve/mpi/3.4.0/bin/mpic++ -o Checker Checker.cpp
{
    char PP[50];
    char IS[50];

    strcpy(PP, "time /opt/nec/ve/bin/mpirun -n 8 /uhome/a01431/ADIOS2/ADIOS2-Examples/VE_Try/bin/adios2-gray-scott settings-files.json && time /opt/nec/ve/bin/mpirun -vh -n 1 ~/miniconda3/envs/adios2/bin/python gsplot.py -i gs.bp" );
    strcpy(IS, "time /opt/nec/ve/bin/mpirun -n 8 /uhome/a01431/ADIOS2/ADIOS2-Examples/VE_Try/bin/adios2-gray-scott settings-staging.json && time /opt/nec/ve/bin/mpirun -vh -n 1 ~/miniconda3/envs/adios2/bin/python gsplot.py -i gs.bp" );
    
    Settings settings = Settings::from_json("settings-files.json");
    if((settings.L*settings.L*settings.L)*2*8*0.000000001*(settings.steps/settings.plotgap) >   1){
        system(PP);
        printf("It is PP");
    }else{
        system(IS);
        printf("It is IS");
    }

    return(0);
} 
