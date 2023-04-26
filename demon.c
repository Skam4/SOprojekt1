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

//wypisanie wyniku polecenia do pliku
void stdout_plik(int wypisanie, char komendy[][100], int zadanie)
{
    dup2(wypisanie, STDOUT_FILENO);
    close(wypisanie);
    execlp(komendy[zadanie], komendy[zadanie], NULL);
    perror("execlp");
    exit(1);
}

//wypisanie bledu polecenia do pliku
void stderr_plik()
{
    
}

int main(int argc, char *argv[]) 
{
    pid_t pid, sid;
	
    // Pobieramy taskfile i outfile
    if(argc != 3) 
    {
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

    char znak; //kolejne odczytywane znaki
    char odczytany_bajt; //pomocnicza zmienna do sprawdzania, czy czytanie nadal trwa
    int i=0; //pomocnicza zmienna do liczenia ktory z kolei znak wczytujemy

    while (odczytany_bajt = read(zadania, &znak, 1) > 0)
    {
        if(znak == '\n')
        {
            linia[i] = '\0'; //Przetwarzanie linii tekstu
            i=0;

            wartosc = strtok(linia, ":");
            zadania_tab[ilosc_zadan][0] = atoi(wartosc); //atoi zamienia char na int
            
            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][1] = atoi(wartosc);
            
            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][2] = atoi(wartosc);
            strcpy(komendy[ilosc_zadan], wartosc); //Zapisywanie polecenia jako char w tablicy komend
            
            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][3] = atoi(wartosc);
            
            ilosc_zadan++;
        }
        else
        {
            linia[i++] = znak;
        }
    }

    //Sortowanie chronologiczne instrukcji
    Sortowanie(zadania_tab, ilosc_zadan, komendy);
    
    int zadanie = 0; //Oblicza ilosc zrobionych zadan
    int kod_wyjscia = 0;
    
    pid = fork(); //Fork- funkcja uruchamia nowy proces potomny
    
    openlog("Proces potomny", LOG_PID, LOG_USER); //Inicjalizacja log
    
    if (pid < 0) 
    {
        printf("Nie udalo sie utworzyc procesu potomnego");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) 
    {
    	//Proces potomny dziala w petli sprawdzajac, czy juz jest czas na wykonanie zadania
    	for(;;)
    	{
            time_t now = time(NULL); //Pobieramy czas z systemu
	    	struct tm *local = localtime(&now);
	    	int godzina = local->tm_hour;
	    	int minuta = local->tm_min;
	    	if(zadania_tab[zadanie][0] == godzina && zadania_tab[zadanie][1] == minuta)
	    	{
                pid_t pid2 = fork(); //Proces potomny wykonujacy zadanie
                if (pid2 < 0) 
	    	    {
			        printf("Nie udalo sie utworzyc procesu potomnego dla polecenia %s.", komendy[zadanie]);
			        exit(EXIT_FAILURE);
	    	    }
                else if(pid2==0)
                {
                    pid_t pid3 = fork(); //Proces potomny wywolany w celu wykonania zadania
                    if (pid3 < 0) 
                    {
                        printf("Nie udalo sie utworzyc procesu potomnego dla polecenia %s.", komendy[zadanie]);
			            exit(EXIT_FAILURE);
                    } 
                    else if (pid3 == 0) 
                    {
                        syslog(LOG_INFO, "Uruchomiono polecenie %s z parametrem %d o godzinie %d:%d\n", komendy[zadanie], zadania_tab[zadanie][3], zadania_tab[zadanie][0], zadania_tab[zadanie][1]);

                        char buf[100];
                        snprintf(buf, sizeof(buf), "\nUruchomiono polecenie %s z parametrem %d o godzinie %d:%d\n\n", komendy[zadanie], zadania_tab[zadanie][3], zadania_tab[zadanie][0], zadania_tab[zadanie][1]);
                        write(wypisanie, buf, strlen(buf));
                        
                        //Kolejne parametry
                        if(zadania_tab[zadanie][3] == 0)
                        {
                            stdout_plik(wypisanie, komendy, zadanie);
                        }
                        else if(zadania_tab[zadanie][3] == 1)
                        {
                            stderr_plik();
                        }
                        else if(zadania_tab[zadanie][3] == 2)
                        {
                            stdout_plik(wypisanie, komendy, zadanie);
                            stderr_plik();
                        }
                        else
                        {
                            snprintf(buf, sizeof(buf), "\nNieprawidlowa wartosc parametru\n\n");
                            write(wypisanie, buf, strlen(buf));
                        }
                    }
                    kod_wyjscia = close(wypisanie);
                    syslog(LOG_INFO, "Zakonczono polecenie %s z parametrem %d o godzinie %d:%d z kodem wyjscia %d\n", komendy[zadanie],  zadania_tab[zadanie][3], zadania_tab[zadanie][0], zadania_tab[zadanie][1], kod_wyjscia);
                    exit(EXIT_SUCCESS);
                }
                zadanie++;
                if(zadanie==ilosc_zadan)
                {
                    //zamkniecie pliku dodac
                    exit(EXIT_SUCCESS);
                }
            }
            sleep(30);
        }
    }
    return 0;
}
