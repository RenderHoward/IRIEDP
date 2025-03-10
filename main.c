#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <math.h>

#include "irisub.h"

int edp( float lat, float lon, int yyyy, int mm, int dd, float hour, HeightData * const data )
{
    float out_array[1000][20], out_list[100], min = (float)data->min, max = (float)data->max;

    int jw[50], jmag = 0, mmdd = 100*mm + dd;

    for( int i=0; i<50; i++ )
        jw[i] = true;

    jw[4] = jw[5] = jw[6] = jw[21] = jw[23] = jw[28] = jw[29] = jw[30] = jw[33] = jw[35] = jw[39] = jw[40] = jw[47] = false;

    float step = (data->max - data->min) / (data->num_samples - 1);

    int code = iri_sub_c( &jw, &jmag, &lat, &lon, &yyyy, &mmdd, &hour, &min, &max, &step, &out_array, &out_list);

    for( int i = 0; i<data->num_samples; i++ )
        data->values[i] = out_array[i][0];

    return 0;
}

char *g_monthnames[13] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int main( int argc, char **argv )
{
    if( argc != 10 )
    {
        printf
        (
            "Argument error \n"
            " USAGE: (9 numeric fields) \n "
            " mm dd yyyy hr lat lon start_height stop_height, num_meas\n\n"
            " Lat, Lon in decimal degrees west longitudes negative\n"
            " Height in km\n\n"
        );

        return -1;
    }

    int n = atoi(argv[9]);

    double start = atof(argv[7]), stop = atof(argv[8]);

    HeightData data = { start, stop, n };

    int mm = atoi(argv[1]), dd = atoi(argv[2]), yyyy = atoi(argv[3]);

    double hr = atof(argv[4]), lat = atof(argv[5]) ,lon = atof(argv[6]);

    int ret = edp(lat, lon, yyyy, mm, dd, hr, &data);

    char buffer[1024];

    sprintf(buffer, "Lat: %6.3f  Lon: %6.3f %s %d, %d  %5.2f UTC",
            lat, lon, g_monthnames[mm], dd, yyyy, hr);

    if( ret == 0 )
           ret = launchplot(&data, buffer);
            
    return ret;
}

int launchplot( HeightData const * const data, char const * const legend_str )
{
    int pid, status;

    char *datafile_name = "gnuplot.dat", *plotfile_name = "gnuplot.plt";

    double step = (data->max - data->min) / (data->num_samples - 1);

    FILE *fp;

    fp = fopen (datafile_name, "w");

    for( int i = 0; i < data->num_samples; i++ )
        fprintf (fp, "%.6e %.6e\n", data->values[i], data->min + i * step );

    fclose (fp);

    fopen (plotfile_name, "w");

    fprintf (fp, "set xlabel 'Electron Density (e^-/m^3) '\n"
                "set ylabel 'Height (km)'\n"
                "set title 'Electron Density Plot'\n"
                "set style data lines\n"
                "plot \"%s\" using 1:2 title \"%s\" lw 3 linecolor rgb \"blue\"\n"
                "quit\n", datafile_name, legend_str);
    fclose (fp);

    char *args[] = { "gnuplot", "-p", plotfile_name, NULL };

    pid = fork();

    if (pid < 0)
    {
        fprintf (stderr, "Failed to Fork.\n");
        return 1;
    }
    else if (pid == 0)
    {
        if (execvp (*args, args) == -1)
        {
            fprintf (stderr, "Gnuplot failed to open.\n");
            _exit (EXIT_FAILURE);
        }
    }

    waitpid (pid, &status, 0);

    return 0;
}
