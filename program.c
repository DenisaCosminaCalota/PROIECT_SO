#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define MAXIM 100


typedef struct{
    int reportId;
    char inspectorName[MAXIM];
    struct GPS{
        float X;
        float Y;
    }GPSCoordinates;
    char issueCategory[MAXIM];
    int severityLevel;
    time_t timestamp;
    char descriptionText[MAXIM];
}ReportFile;


int main(int argc, char *argv[])
{
    char comanda[MAXIM];
    FILE* f;
    return 0;

}
