#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#define MAXIM 100

typedef enum Role{
    Role_Manager,
    Role_Inspector,
    Role_None,
}Role;

typedef struct{
    int reportId;
    char inspectorName[MAXIM];
    struct GPS{
        double latitude;
        double longitude;
    }GPSCoordinates;
    char issueCategory[MAXIM];
    int severityLevel;
    time_t timestamp;
    char descriptionText[MAXIM];
}ReportFile;

typedef struct PermissionBits{

    mode_t ReadBit;
    mode_t WriteBit;
    mode_t ExecuteBit;

}RoleBits;

int parse_condition(const char *input, char *field, char *op, char *value) {
    int count = sscanf(input, "%[^:]:%[^:]:%s", field, op, value);
    return (count == 3);
}

int match_condition(ReportFile *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val_comp = atoi(value);
        if (strcmp(op, "==") == 0) return r->severityLevel == val_comp;
        if (strcmp(op, "!=") == 0) return r->severityLevel != val_comp;
        if (strcmp(op, ">") == 0)  return r->severityLevel > val_comp;
        if (strcmp(op, ">=") == 0) return r->severityLevel >= val_comp;
        if (strcmp(op, "<") == 0)  return r->severityLevel < val_comp;
        if (strcmp(op, "<=") == 0) return r->severityLevel <= val_comp;
    }
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->issueCategory, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->issueCategory, value) != 0;
    }
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspectorName, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspectorName, value) != 0;
    }
    else if (strcmp(field, "timestamp") == 0) {
        long long val_ts = atoll(value);
        if (strcmp(op, "==") == 0) return (long long)r->timestamp == val_ts;
        if (strcmp(op, ">") == 0)  return (long long)r->timestamp > val_ts;
        if (strcmp(op, "<") == 0)  return (long long)r->timestamp < val_ts;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    int fd = open("reports.dat", O_RDONLY);
    if (fd < 0) { perror("Eroare deschidere"); return 1; }

    ReportFile r;
    char f[MAXIM], o[MAXIM], v[MAXIM];

    if (!parse_condition(argv[7], f, o, v)) {
        printf("Format filtru invalid.\n");
        close(fd);
    return 1;
    }

    while (read(fd, &r, sizeof(ReportFile)) > 0) {
        if (match_condition(&r, f, o, v)) {
            printf("ID: %d | Cat: %s | Sev: %d | Desc: %s\n",
                r.reportId, r.issueCategory, r.severityLevel, r.descriptionText);
        }
    }
    close(fd);
}
