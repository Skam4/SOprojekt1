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

void parametr_0(int zadania_tab[][4], char komendy[][100], int zadanie, FILE* wypisanie)
{
    printf("%d:%d %s %d \n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);
    fprintf(wypisanie, "%d:%d %s %d \n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);

    FILE *fp;
    char wynik[1024];
    fp = popen(komendy[zadanie], "r");  // wywołanie polecenia w celu zapisania jego tresci
    
    if (fp == NULL) {
        printf("Błąd podczas wywoływania polecenia.\n");
    }

    // Odczytanie wyniku z polecenia i zapisanie go do zmiennej
    while (fgets(wynik, sizeof(wynik), fp) != NULL) {}

    pclose(fp);  // zamknięcie strumienia

    fprintf(wypisanie, "%s\n", wynik); //wypisanie wyniku polecenia do pliku (na razie wypisuje tylko ostatnie słowo z wyniku danego polecenia, trzeba pokminić nad tym jeszcze)
}

void parametr_1(int zadania_tab[][4], char komendy[][100], int zadanie, FILE* wypisanie)
{
    printf("%d:%d %s %d \n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);
    fprintf(wypisanie, "%d:%d %s %d \n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);
}

void parametr_2(int zadania_tab[][4], char komendy[][100], int zadanie, FILE* wypisanie)
{
    printf("%d:%d %s %d \n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);
    fprintf(wypisanie, "%d:%d %s %d \n", zadania_tab[zadanie][0], zadania_tab[zadanie][1], komendy[zadanie], zadania_tab[zadanie][3]);
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
    
    //Dalej
    
    
    int zadanie = 0;
    int kod_wyjscia = 0;
    
    pid = fork(); //Fork- funkcja uruchamia nowy proces potomny
    
    openlog("proces potomny", LOG_PID, LOG_USER); //Inicjalizacja log
    
    if (pid < 0) {
        printf("Nie udalo sie utworzyc procesu potomnego dla polecenia:"); //Trzeba dodać które
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
    
    	//Musimy sprawdzać czy już jest czas i wtedy lecimy do ifów
    	
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
                pid_t pid2 = fork(); //proces potomny wykonujacy zadanie
                if (pid2 < 0) 
	    	    {
			        printf("Nie udalo sie utworzyc procesu potomnego dla polecenia.");
			        exit(EXIT_FAILURE);
	    	    }
                else if(pid2==0)
                {
                    syslog(LOG_INFO, "Uruchomiono polecenie %s z parametrem %d o godzinie %d:%d\n", komendy[zadanie], zadania_tab[zadanie][3], zadania_tab[zadanie][0], zadania_tab[zadanie][1]);
                    if(zadania_tab[zadanie][3] == 0)
                    {
                        parametr_0(zadania_tab, komendy, zadanie, wypisanie);
                    }
                    if(zadania_tab[zadanie][3] == 1)
                    {
                        parametr_1(zadania_tab, komendy, zadanie, wypisanie);
                    }
                    if(zadania_tab[zadanie][3] == 2)
                    {
                        parametr_2(zadania_tab, komendy, zadanie, wypisanie);
                    }

                    /*kod_wyjscia = system(komendy[zadanie]);*/
                    
                    //ZAPISYWANIE KODU WYJŚCIA (wczesniej bylo to zrobione za pomoca funkcji system(), ale przez to wypisywalo sie do terminala a te popen podobno sprawia, ze tak nie bedzie, aczkolwiek kiedy wyskoczy błąd to nadal sie wypisuje do terminala, co jest troche problematyczne)
                    char output[1024];
                    FILE *fp;
                    fp = popen(komendy[zadanie], "r");
                    if (fp == NULL) 
                    {
                        printf("Błąd podczas wywoływania polecenia.\n");
                        return 1;
                    }
                    // Odczytanie wyniku z polecenia i zapisanie go do zmiennej
                    while (fgets(output, sizeof(output), fp) != NULL) {}
                    kod_wyjscia = pclose(fp);

                    syslog(LOG_INFO, "Zakonczono polecenie %s z parametrem %d o godzinie %d:%d z kodem wyjscia %d\n", komendy[zadanie],  zadania_tab[zadanie][3], zadania_tab[zadanie][0], zadania_tab[zadanie][1], kod_wyjscia);
                    
                    exit(EXIT_SUCCESS); //zakonczenie dzialania procesu potomnego wykonujacego zadanie
                }
                zadanie++;
                if(zadanie==ilosc_zadan)
        	        exit(EXIT_SUCCESS); //jezeli zrobil juz wszystkie zadania to sie konczy
		
		//nie musi być start i goto bo mamy wszystko w for. A for bardziej czytelny jest
	    	}
    	}
    }
    return 0;
}