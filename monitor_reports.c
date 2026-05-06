#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#define PID_FILE ".monitor_pid"

volatile sig_atomic_t keep_running =1;

void handle_signal(int signal){
    if(signal == SIGINT){
        char message[]="[MONITOR] SIGINT primit cu succes. Se inchide fisierul.";
        write(STDOUT_FILENO, message, strlen(message));
        keep_running = 0;
    }
    if(signal == SIGUSR1){
        char message[]="[MONITOR] Un nou raport a fost detectat.";
        write(STDOUT_FILENO, message, strlen(message));
    }
}
int main(){
    struct sigaction action;
    memset(&action, 0x00, sizeof(struct sigaction));
    action.sa_handler = signal;

  if (sigaction(SIGUSR1, &action, NULL) < 0)
    {
      perror("sigaction SIGUSR1 ignore");
      exit(-1);
    }
  if (sigaction(SIGUSR2, &action, NULL) < 0)
    {
      perror("sigaction SIGUSR1 ignore");
      exit(-1);
    }
    pid_t pid =getpid();
    FILE *f_pid = open (PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(f_pid == -1)
    {
        printf("Eroare la crearea fisierului");
        exit(EXIT_FAILURE);
    }
    char pidString[16];

    int lungime = snprintf(pidString, sizeof(pidString), "%d\n", pid);
    write(f_pid, pidString, lungime);
    close(f_pid);

return 0;

}
