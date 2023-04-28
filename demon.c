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
#include <signal.h>
#include <setjmp.h>

//Funkcja sortująca wczytane polecenia sortowaniem przez wstawianie
void Sortowanie(int zadania[][4], int n, char komendy[][100]) {
    int i, j, k, pom[4];
    char pom2[100][100];
    for (i = 0; i < n; i++) {
        j = i - 1;
        //Kopiowanie i-tego wiersza do zmiennej pom
        for (k = 0; k < 4; k++) {
            pom[k] = zadania[i][k];
        }
        strcpy(pom2[i],komendy[i]);
        //Przesuwanie wiekszych elementów w prawo
        while (j >= 0 && (zadania[j][0] > pom[0] || (zadania[j][0] == pom[0] && zadania[j][1] > pom[1]))) {
            for (k = 0; k < 4; k++) {
                zadania[j + 1][k] = zadania[j][k];
            }
            strcpy(komendy[j + 1], komendy[j]); //Te same operacje wykonywane są na tablicy komend, która pr
            j--;
        }
        // wstawienie i-tego elementu w odpowiednie miejsce
        for (k = 0; k < 4; k++) {
            zadania[j + 1][k] = pom[k];
        }
         strcpy(komendy[j + 1], pom2[i]);
    }
}

//Wypisywanie wyniku polecenia i/lub błędu do pliku
void stdout_stderr(int wypisanie, char komendy[][100], int zadanie, int parametr)
{
    int dev_null = open("/dev/null", O_WRONLY); //Otwarcie urządzenia /dev/null w celu wypisania do niego niepotrzebnych na standardowym wyjściu informacji
    if(parametr==0)
    {
        dup2(wypisanie, STDOUT_FILENO); //Przekierowanie standardowego wyjścia na wypisywanie do pliku
        dup2(dev_null, STDERR_FILENO); //Przekierowanie wyjścia błędu do urządzenia /dev/null
    }
    if(parametr==1)
    {
        dup2(dev_null, STDOUT_FILENO); //Przekierowanie standardowego wyjścia do urządzenia /dev/null
        dup2(wypisanie, STDERR_FILENO); //Przekierowanie wyjścia błędu do urządzenia /dev/null
    }
    if(parametr==2)
    {
        dup2(wypisanie, STDOUT_FILENO); //Przekierowanie standardowego wyjścia na wypisywanie do pliku
        dup2(wypisanie, STDERR_FILENO); //Przekierowanie wyjścia błędu na wypisywanie do pliku
    }
    close(wypisanie);
    close(dev_null);

    char* arg[10]; //Tablica przechowująca kolejne argumenty przekazane w tablicy komendy
    int i=0;

    while(i<10)
    {
        if(i==0)
        {
            arg[i] = strtok(komendy[zadanie]," "); //Pierwszy argument przyjmuje wartość tablicy komend aż do pierwszego znaku pustego, którymi argumenty są rozdzielone
        }
        else
        {
            arg[i] = strtok(NULL," "); //Każdy kolejny argument przyjmuje kolejne wartości przekazane w tablicy komend
        }
        i++;
    }

    execlp(arg[0], arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], NULL); //Funkcja execpl wywołuje proces, który wykonuje przekazane polecenie
    perror("Error"); //Funkcja perror zwraca błąd powstały z wykonania polecenia (jeżeli takowy błąd wystąpił)
    exit(1);
}

//Funkcja zwracająca czas do zadania w sekundach
int CzasDoZadania(int hour, int minutes)
{
	time_t now = time(NULL); //pobieramy czas z systemu
	struct tm *local = localtime(&now);
	int godzina = local->tm_hour;
	int minuta = local->tm_min;
	int czas1, czas2;
	czas1 = hour*3600 + minutes*60;
	czas2 = godzina*3600 + minuta*60;
	return czas1-czas2 > 1 ? czas1-czas2 : 1;
}

jmp_buf restart_point;

int **tasks; //tablica z zadaniami, potrzebna do sygnałów
char *taski; //tablica przetrzymująca polecenia jako napisy
int wiersze; //Funkcja zapamiętująca ilość wierszy w tablicy tasks


//Funkcja obsługująca sygnał SIGINT
void sigint_handler(int sig)
{

    exit(EXIT_SUCCESS);
}

//Funkcja obsługująca sygnał SIGUSR1
void sigusr1_handler(int sig) 
{
    // ustawienie wartości powrotu dla longjmp()
    int return_value = 1;
    // wywołanie longjmp() z przygotowanym punktem skoku
    longjmp(restart_point, return_value);
}

//Funkcja obsługująca sygnał SIGUSR2
void sigusr2_handler(int sig)
{
    syslog(LOG_INFO, "Lista zadań pozostala do wykonania:\n");
    for(int i=0 ; i < wiersze ; i++)
    {
        if(CzasDoZadania(tasks[i][0], tasks[i][1]) >= 1)
        {
            syslog(LOG_INFO, "Polecenie %s o godzinie %d:%d\n", taski[i], tasks[i][0], tasks[i][1]);
        }
    }
}

//Funkcja która wywołuje się podczas wywołania exit
void czysc()
{
    //Czyścimy pamięć globalnej tablicy tasks
    for(int i=0 ; i < wiersze ; i++)
    free(tasks[i]);

    //Czyścimy pamięć globalnej tablicy taski
    free(taski);
}

//Funkcja main
int main(int argc, char *argv[]) 
{
    //if do którego funkcja wchodzi, gdy wywołamy SIGUSR1
    if (setjmp(restart_point) != 0) {
        // kod do wykonania w momencie powrotu z sigusr1_handler
        printf("SIGUSR1 WYKONANY!\n");
    }

    //Tworzymy identyfikator
    pid_t pid;
	
    // Pobieramy taskfile i outfile
    if(argc != 3) 
    {
        printf("Za duzo lub za malo wprowadzonych danych!\n");
        return 1;
    }
    
    //Do taskfile przydzielamy argument 1
    char *taskfile = argv[1];
    //Do outfile przydzielamy argument 2
    char *outfile = argv[2];
    
    //otwieranie pliku taskfile i outfile
    int zadania = open(taskfile, O_RDONLY);
    int wypisanie = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    
    //Funkcja uruchamiająca się przy wywołaniu exit
    //atexit(czysc);

    //Obsługa błędów plików
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

    close(zadania); //

    time_t now = time(NULL); //pobieramy czas z systemu
	struct tm *local = localtime(&now);
	int godzina = local->tm_hour;
	int minuta = local->tm_min;

    //tutaj trzeba zrobić prawdziwą tablice zadania_tab
    int zadania_tab2[1000][4]; //mozna inaczej nazwac
    char komendy2[1000][100];
    int pomoc = 0;

    for(int i = 0; i < ilosc_zadan ; i++)
    {
        if(zadania_tab[i][0] > godzina || (zadania_tab[i][0] == godzina && zadania_tab[i][1] >= minuta))
        {
            zadania_tab2[pomoc][0] = zadania_tab[i][0];
            zadania_tab2[pomoc][1] = zadania_tab[i][1];
            zadania_tab2[pomoc][2] = zadania_tab[i][2]; 
            strcpy(komendy2[pomoc], komendy[i]);
            zadania_tab2[pomoc][3] = zadania_tab[i][3];

            printf("zadania_tab[0]: %d     godzina: %d     zadania_tab[1]: %d     minuta: %d\n", zadania_tab[i][0], godzina, zadania_tab[i][1], minuta);
            printf("pomoc: %d   i: %d\n", pomoc, i);
            printf("%d : %d\n", zadania_tab2[pomoc][0], zadania_tab2[pomoc][1]);

            pomoc++;
        }
    }

    pomoc--;

    for(int i=0 ; i<pomoc ; i++)
    {
        printf("godzina: %d      minuta: %d\n", zadania_tab2[i][0], zadania_tab2[i][1]);
    }


    //alokacja pamięci tablicy tasks
    tasks = (int **) malloc(pomoc+1 * sizeof(int *));
    
    //alokacja pamięci tablicy taski
    taski = (char *) malloc(pomoc+1 * sizeof(char));

    //wiersze = pomoc;

    // przypisanie zadania_tab2 pod tasks
    for (int i = 0; i <= pomoc; i++) 
    {
        tasks[i] = (int *) malloc(4 * sizeof(int));
        tasks[i] = zadania_tab2[i];
        taski[i] = komendy2[i];
    }

    //Sortowanie chronologiczne instrukcji
    Sortowanie(*zadania_tab2, pomoc, *komendy2);
    
    int zadanie = 0; //odlicza ilość zrobionych zadań
    int kod_wyjscia = 0;
    int sekundy = 0; //pobiera za ile sekund musi się obudzić demon
    
    pid = fork(); //Fork- funkcja uruchamia nowy proces potomny
    
    openlog("Proces potomny", LOG_PID, LOG_USER); //Inicjalizacja log

    //SIGINT inicjalizacja
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
        perror("Nie udało się zarejestrować obsługi sygnału SIGINT");
        exit(EXIT_FAILURE);
    }

    //SIGUSR1 inicjalizacja
    if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR)
    {
        perror("Nie udało się zarejestrować obsługi sygnału SIGUSR1");
        exit(EXIT_FAILURE);
    }

    //SIGUSR2 inicjalizacja
    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR)
    {
        perror("Nie udało się zarejestrować obsługi sygnału SIGUSR2");
        exit(EXIT_FAILURE);
    }
    
    //Przypadek błędnego utworzenia procesu
    if (pid < 0) 
    {
        printf("Nie udalo sie utworzyc procesu potomnego");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) 
    {

    	//Proces dziala w petli sprawdzajac, czy juz jest czas na wykonanie zadania
    	while(pomoc >= zadanie)
    	{
            sekundy = CzasDoZadania(zadania_tab2[zadanie][0], zadania_tab2[zadanie][1]);
	    	printf("Demon obudzi sie za: %d sekund\n", sekundy);
	    	sleep(sekundy);

            pid_t pid2 = fork(); //Proces potomny wykonujacy zadanie
            if (pid2 < 0) 
	    	{
			    printf("Nie udalo sie utworzyc procesu potomnego dla polecenia %s.", komendy2[zadanie]);
			    exit(EXIT_FAILURE);
	    	}
            else if(pid2==0)
            {
                syslog(LOG_INFO, "Uruchomiono polecenie %s z parametrem %d o godzinie %d:%d\n", komendy2[zadanie], zadania_tab2[zadanie][3], zadania_tab2[zadanie][0], zadania_tab2[zadanie][1]);

                char buf[100];
                snprintf(buf, sizeof(buf), "\nUruchomiono polecenie %s z parametrem %d o godzinie %d:%d\n\n", komendy2[zadanie], zadania_tab2[zadanie][3], zadania_tab2[zadanie][0], zadania_tab2[zadanie][1]);
                write(wypisanie, buf, strlen(buf));
                        
                //Kolejne parametry
                if(zadania_tab2[zadanie][3] == 0)
                {
                    stdout_stderr(wypisanie, komendy2, zadanie, 0);
                }
                else if(zadania_tab2[zadanie][3] == 1)
                {
                    stdout_stderr(wypisanie, komendy2, zadanie, 1);
                }
                else if(zadania_tab2[zadanie][3] == 2)
                {
                    stdout_stderr(wypisanie, komendy2, zadanie, 2);
                }
                else
                {
                    snprintf(buf, sizeof(buf), "Nieprawidlowa wartosc parametru\n\n");
                    write(wypisanie, buf, strlen(buf));
                }
                    
                kod_wyjscia = close(wypisanie);
                syslog(LOG_INFO, "Zakonczono polecenie %s z parametrem %d o godzinie %d:%d z kodem wyjscia %d\n", komendy2[zadanie],  zadania_tab2[zadanie][3], zadania_tab2[zadanie][0], zadania_tab2[zadanie][1], kod_wyjscia);
                exit(EXIT_SUCCESS);
            }
            zadanie++;
            if(zadanie==ilosc_zadan)
            {
                //zamkniecie pliku dodac
                exit(EXIT_SUCCESS);
            }
        }
    }
    exit(EXIT_SUCCESS);
}