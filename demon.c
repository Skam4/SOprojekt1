#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <string.h>
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

/*
void sigint_wywolanie(int sig)
{
//Można coś napisać
printf("Sigint wywołane\n"); //Czemu \n nie działa?
exit(0);
} */

int main(int argc, char *argv[]) {
    pid_t pid, sid;
	
    // Pobieramy taskfile i outfile
    if(argc != 3) {
        printf("Za duzo lub za malo wprowadzonych danych!\n");
        return 1;
    }

    char *taskfile = argv[1];
    char *outfile = argv[2];

    //otwieranie pliku taskfile i outfile
    FILE *zadania = fopen(taskfile, "r");
    FILE *wypisanie = fopen(outfile, "w");

    if(zadania == NULL)
    {
        printf("Nie mozna otworzyc pliku taskfile!\n");
        return 1;
    }

    if(wypisanie == NULL)
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

    ////// linia=bufor, 100 = maksymalna dlugosc
    while (fgets(linia, 100, zadania) && ilosc_zadan < 1000)
    {
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
    }

    //Sortowanie chronologiczne instrukcji

    Sortowanie(zadania_tab, ilosc_zadan, komendy);

    //Ustawienie usługi syganłu SIGINT (chyba)

    //signal(SIGINT, sigint_wywolanie); //Do SIGINT przypisujemy wywołanie funkcji sigint_wywolanie
    //^ nie kompiluje sie

    //Dalej

    int zadanie = 0;

    pid = fork(); //Fork- funkcja uruchamia nowy proces potomny
    if (pid < 0) {
        printf("Nie udalo sie utworzyc procesu potomnego dla polecenia:"); //Trzeba dodać które
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
    	//Musimy sprawdzać czy już jest czas i wtedy lecimy do ifów
    	start: //etykieta startowa do ktorej bedzie wracac
    	for(;;)
    	{
	    	time_t now = time(NULL); //pobieramy czas z systemu
	    	struct tm *local = localtime(&now);
	    	int godzina = local->tm_hour;
	    	int minuta = local->tm_min;
	    	if(zadania_tab[zadanie][0] == godzina && zadania_tab[zadanie][1] == minuta)
	    	{
                printf("czas na %d:%d %s %d\n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);
                //^powiadomienie takie do testow
	    		break;
	    	}
	    	sleep(10);
    	}
        
        fprintf(wypisanie, "%d:%d %s %d \n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);
        if(zadania_tab[zadanie][3] == 0)
        {
		    fprintf(wypisanie, "0\n");
            zadanie++;

            if(zadanie==ilosc_zadan)
                exit(EXIT_SUCCESS); //jezeli zrobil juz wszystkie zadania to sie konczy
            else
                goto start; //a jezeli nie to leci do etykiety startowej
        }

        if(zadania_tab[zadanie][3] == 1)
        {
		    fprintf(wypisanie, "1\n");
            zadanie++;
            if(zadanie==ilosc_zadan)
                exit(EXIT_SUCCESS);
            else
                goto start;
        }

        if(zadania_tab[zadanie][3] == 2)
        {
		    fprintf(wypisanie, "2\n");
            zadanie++;
            if(zadanie==ilosc_zadan)
                exit(EXIT_SUCCESS);
            else
                goto start;
        }
    }
    return 0;
}
