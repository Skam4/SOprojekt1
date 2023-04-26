#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

//Funkcja sortowania przez wstawianie
void Sortowanie(int zadania[][4], int n, char komendy[][100]) {
    int i, j, k, pom[4];
    char pom2[100][100];
    for (i = 0; i < n; i++) {
        j = i - 1;
        // kopiowanie i-tego wiersza do zmiennej pom
        for (k = 0; k < 4; k++) {
            pom[k] = zadania[i][k];
        }
        strcpy(pom2[i],komendy[i]);
        // przesuwanie wiekszych elementów w prawo
        while (j >= 0 && (zadania[j][0] > pom[0] || (zadania[j][0] == pom[0] && zadania[j][1] > pom[1]))) {
            for (k = 0; k < 4; k++) {
                zadania[j + 1][k] = zadania[j][k];
            }
            strcpy(komendy[j + 1], komendy[j]);
            j--;
        }
        // wstawienie i-tego elementu w odpowiednie miejsce
        for (k = 0; k < 4; k++) {
            zadania[j + 1][k] = pom[k];
        }
         strcpy(komendy[j + 1], pom2[i]);
    }
}

int main(int argc, char *argv[]) 
{
    pid_t pid, sid;
	
    // Pobieramy taskfile i outfile
    if(argc != 3) {
        printf("Za duzo lub za malo wprowadzonych danych!\n");
        return 1;
    }
    
    char *taskfile = argv[1];
    char *outfile = argv[2];
    
    //otwieranie pliku taskfile i outfile
    int zadania = open(taskfile, O_RDONLY);
    int wypisanie = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    
    if(zadania == -1)
    {
        printf("Nie mozna otworzyc pliku taskfile!\n");
        return 1;
    }
    if(wypisanie == -1)
    {
        printf("Nie mozna otworzyc pliku outfile!\n");
        return 1;
    }
    
    //Wczytywanie danych z pliku taskfile
    char linia[100]; //bufor
    char *wartosc; //pomocniczo zapisujemy poszczegolne wartosci
    int ilosc_zadan = 0; //pomocniczo liczymy ilosc zadan
    int zadania_tab[1000][4]; //tablica z oddzielonymi wartosciami
    char komendy[100][100]; //tablica char do przechowywania komend

    char znak;
    char odczytany_bajt;
    int i=0;

    ////// linia=bufor, 100 = maksymalna dlugosc
    while (odczytany_bajt = read(zadania, &znak, 1) > 0)
    {
        if(znak == '\n')
        {
            linia[i] = '\0'; // przetwarzanie linii tekstu
            i=0;
            wartosc = strtok(linia, ":");
            zadania_tab[ilosc_zadan][0] = atoi(wartosc); //atoi zamienia char na int
            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][1] = atoi(wartosc);
            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][2] = atoi(wartosc);
            strcpy(komendy[ilosc_zadan], wartosc); //zapisywanie napisu jako char w tablicy komendy, bo wczesniej zamienia sie w int i jest to bez sensu
            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][3] = atoi(wartosc);
            ilosc_zadan++;
            printf("\nilosc_zadan: %d\n",ilosc_zadan);
        }
        else
        {
            linia[i++] = znak;
        }
    }

    //Sortowanie chronologiczne instrukcji
    Sortowanie(zadania_tab, ilosc_zadan, komendy);
    
    int zadanie = 0;
    int kod_wyjscia = 0;
    
    pid = fork(); //Fork- funkcja uruchamia nowy proces potomny
    
    openlog("Proces potomny", LOG_PID, LOG_USER); //Inicjalizacja log
    
    if (pid < 0) {
        printf("Nie udalo sie utworzyc procesu potomnego dla polecenia:"); //Trzeba dodać które
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) 
    {
    	//Musimy sprawdzać czy już jest czas i wtedy lecimy do ifów
    	for(;;)
    	{
            time_t now = time(NULL); //pobieramy czas z systemu
	    	struct tm *local = localtime(&now);
	    	int godzina = local->tm_hour;
	    	int minuta = local->tm_min;
	    	if(zadania_tab[zadanie][0] == godzina && zadania_tab[zadanie][1] == minuta)
	    	{
                pid_t pid2 = fork(); //proces potomny wykonujacy zadanie
                if (pid2 < 0) 
	    	    {
			        printf("Nie udalo sie utworzyc procesu potomnego dla polecenia %s.", komendy[zadanie]);
			        exit(EXIT_FAILURE);
	    	    }
                else if(pid2==0)
                {
                    pid_t pid3 = fork();
                    if (pid3 < 0) 
                    {
                        perror("fork");
                        exit(1);
                    } 
                    else if (pid3 == 0) 
                    {
                        dup2(wypisanie, STDOUT_FILENO);
                        close(wypisanie);
                        execlp(komendy[zadanie], komendy[zadanie], NULL);
                        perror("execlp");
                        exit(1);
                    } 
                    else 
                    {
                        int status;
                        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        printf("Polecenie wykonane poprawnie.\n");
                        } else {
                        printf("Błąd wykonania polecenia.\n");
                        }
                        close(wypisanie);
                        exit(0);
                    }
                }
                zadanie++;
                if(zadanie==ilosc_zadan)
                {
                    exit(EXIT_SUCCESS);
                }
            }
            sleep(30);
        }
    }
    return 0;
}
