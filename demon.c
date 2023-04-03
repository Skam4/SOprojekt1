#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <string.h>

//Funkcja sortowania przez wstawianie
void Sortowanie(int zadania[][4], int n) {
    int i, j, k, pom[4];
    for (i = 1; i < n; i++) {
        j = i - 1;
        // kopiowanie i-tego wiersza do zmiennej pom
        for (k = 0; k < 4; k++) {
            pom[k] = zadania[i][k];
        }
        // przesuwanie większych elementów w prawo
        while (j >= 0 && (zadania[j][0] > pom[0] || (zadania[j][0] == pom[0] && zadania[j][1] > pom[1]))) {
            for (k = 0; k < 4; k++) {
                zadania[j + 1][k] = zadania[j][k];
            }
            j--;
        }
        // wstawienie i-tego elementu w odpowiednie miejsce
        for (k = 0; k < 4; k++) {
            zadania[j + 1][k] = pom[k];
        }
    }
}



int main(int argc, char *argv[]) {

    // Pobieramy taskfile i outfile
    if(argc != 3) {
        printf("Za dużo lub za mało wprowadzonych danych!\n");
        return 1;
    }

    char *taskfile = argv[1];
    char *outfile = argv[2];


    //otwieranie pliku taskfile i outfile
    FILE *zadania = fopen(taskfile, "r");
    FILE *wypisanie = fopen(outfile, "w");

    if(zadania == NULL)
    {
        printf("Nie można otworzyć pliku taskfile!\n");
        return 1;
    }

    if(wypisanie == NULL)
    {
        printf("Nie można otworzyć pliku outfile!\n");
        return 1;
    }


    //Wczytywanie danych z pliku taskfile
    char linia[100]; //bufor
    char *wartosc; //pomocniczo zapisujemy poszczególne wartości
    int ilosc_zadan = 0; //pomocniczo liczymy ilość zadań
    int zadania_tab[1000][4]; //tablica z oddzielonymi wartościami

    ////// linia=bufor, 100 = maksymalna długość
    while (fgets(linia, 100, zadania) && ilosc_zadan < 1000) {
        wartosc = strtok(linia, ":");
        zadania_tab[ilosc_zadan][0] = atoi(wartosc); //atoi zamienia char na int
        wartosc = strtok(NULL, ":");
        zadania_tab[ilosc_zadan][1] = atoi(wartosc);
        wartosc = strtok(NULL, ":");
        zadania_tab[ilosc_zadan][2] = atoi(wartosc);
        wartosc = strtok(NULL, ":");
        zadania_tab[ilosc_zadan][3] = atoi(wartosc);
        ilosc_zadan++;
    }



//Sortowanie chronologiczne instrukcji

Sortowanie(zadania_tab, ilosc_zadan);

/* Fork off the parent process */

pid_t pid, sid;

pid = fork();
if (pid < 0) {
exit(EXIT_FAILURE);
}
if (pid > 0) {
exit(EXIT_SUCCESS);
}

/* Change the file mode mask */
umask(0);

/* Open any logs here */
openlog("mydaemon", LOG_PID|LOG_CONS, LOG_USER);

/* Create a new SID for the child process */
sid = setsid();
if (sid < 0) {
/* Log the failure */
syslog(LOG_ERR, "Could not create new SID for child process");
exit(EXIT_FAILURE);
}

/* Change the current working directory */
if ((chdir("/")) < 0) {
/* Log the failure */
syslog(LOG_ERR, "Could not change working directory to /");
exit(EXIT_FAILURE);
}

/* Close out the standard file descriptors */
close(STDIN_FILENO);
close(STDOUT_FILENO);
close(STDERR_FILENO);

/* Daemon-specific initialization goes here */

/* The Big Loop */
while (1) {
/* Do some task here ... */
syslog(LOG_INFO, "Hello, world!");

sleep(60); /* wait 60 seconds */
}

exit(EXIT_SUCCESS);

return 0;
}
