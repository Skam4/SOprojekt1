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
        //Kopiowanie i-tego wiersza do tablicy pomocniczej
        for (k = 0; k < 4; k++) {
            pom[k] = zadania[i][k];
        }
        strcpy(pom2[i],komendy[i]);
        //Przesuwanie wiekszych elementów w prawo
        while (j >= 0 && (zadania[j][0] > pom[0] || (zadania[j][0] == pom[0] && zadania[j][1] > pom[1]))) {
            for (k = 0; k < 4; k++) {
                zadania[j + 1][k] = zadania[j][k];
            }
            strcpy(komendy[j + 1], komendy[j]); //Te same operacje wykonywane są na tablicy komend, która przetrzymuje kolejne parametry <command>
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
        dup2(wypisanie, STDERR_FILENO); //Przekierowanie wyjścia błędu na wypisywanie do pliku
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

//Funkcja zwracająca czas pozostały do wykonania zadania w sekundach
int CzasDoZadania(int hour, int minutes)
{
	time_t now = time(NULL); //Pobranie aktualnego czasu z systemu
	struct tm *local = localtime(&now); //Zastosowanie funkcji localtime do konwertowania wartości now
	int godzina = local->tm_hour; //Pobranie wartości aktualnej godziny
	int minuta = local->tm_min; //Pobranie wartości aktualnej ilości minut
	int czas1, czas2; //Zmienne pomocnocze do liczenia czasu pozostałego do kolejnego wykonywanego zadania
	czas1 = hour*3600 + minutes*60; //Przemnożenie wartości godzin i minut do wartości sekund
	czas2 = godzina*3600 + minuta*60;
	return czas1-czas2 > 1 ? czas1-czas2 : 1; //Jeżeli ilość pozostałych sekund jest większa od 1, to jest przekazywana, a jeżeli jest mniejsza lub róna 1, to zwracana jest 1 sekunda (co w praktyce oznacza, że w tym momencie jest godzina o której należy wykonać kolejne zadanie)
}

jmp_buf restart_point; //Punkt skoku wykorzystywany przy wykryciu sygnału SIGUSR1

int **tasks; //Tablica z zadaniami, potrzebna do obsługi sygnałów
char *taski; //Tablica przetrzymująca polecenia jako napisy
int wiersze; //Tunkcja zapamiętująca ilość wierszy w tablicy tasks

//FUnkcja sigalarm - potrzebna do pause
void handler(int signum)
{
	
}

//Funkcja obsługująca sygnał SIGINT (Ctrl+C)
void sigint_handler(int sig)
{
    exit(EXIT_SUCCESS); //W momencie wykrycia sygnału, proces kończy się
}

//Funkcja obsługująca sygnał SIGUSR1
void sigusr1_handler(int sig) 
{
    //Ustawienie wartości powrotu dla longjmp()
    int return_value = 1;
    //Wywołanie longjmp() z przygotowanym punktem skoku
    longjmp(restart_point, return_value);
}

//Funkcja obsługująca sygnał SIGUSR2
void sigusr2_handler(int sig)
{
    syslog(LOG_INFO, "Lista zadań pozostala do wykonania:\n"); //Wypisanie w logach ilości pozostałych zadań
    for(int i=0 ; i < wiersze ; i++)
    {
        if(CzasDoZadania(tasks[i][0], tasks[i][1]) >= 1) //Wykorzystanie funkcji CzasDoZadania w celu wypisania pozostałych zadań
        {
            syslog(LOG_INFO, "Polecenie %s o godzinie %d:%d\n", taski[i], tasks[i][0], tasks[i][1]); //Wypisywanie w logach systemowych kolejnych pozostałych zadań
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
    //If do którego funkcja wchodzi, gdy wywołamy SIGUSR1
    if (setjmp(restart_point) != 0) {
        //Kod do wykonania w momencie powrotu z sigusr1_handler
        printf("SIGUSR1 WYKONANY!\n");
    }

    //Tworzenie identyfikatora
    pid_t pid;
	
    //Pobieranie taskfile i outfile
    if(argc != 3) 
    {
        printf("Za duzo lub za malo wprowadzonych danych!\n");
        return 1;
    }
    
    //Do taskfile przydzielamy argument 1
    char *taskfile = argv[1];
    //Do outfile przydzielamy argument 2
    char *outfile = argv[2];
    
    //Otwieranie pliku taskfile i outfile
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
    
    int n = 100; // maksymalna liczba zadań, którą chcemy obsłużyć
    int **zadania_tab = (int **)malloc(n * sizeof(int *)); // alokacja pamięci dla wierszy
    for (int i = 0; i < n; i++) {
        zadania_tab[i] = (int *)malloc(4 * sizeof(int)); // alokacja pamięci dla kolumn
    }

    char **komendy = (char **)malloc(n * sizeof(char *)); // alokacja pamięci dla wierszy
    for (int i = 0; i < n; i++) {
        komendy[i] = (char *)malloc(100 * sizeof(char)); // alokacja pamięci dla kolumn
    }

    //Wczytywanie danych z pliku taskfile
    char linia[100]; //Bufor
    char *wartosc; //pomocniczo zapisujemy kolejne wczytywane wartosci
    int ilosc_zadan = 0; //pomocniczo liczymy ilosc zadan do wykonania
    //int zadania_tab[1000][4]; //Tablica z oddzielonymi wartosciami (wczytuje wszystkie parametry i dzieli je według dwukropków)
    //char komendy[100][100]; //Tablica char do przechowywania wczytywanych komend (we wczytywaniu parametrów wczytuje parametr <command>)

    char znak; //Kolejne odczytywane znaki
    char odczytany_bajt; //pomocnicza zmienna do sprawdzania, czy czytanie nadal trwa
    int i=0; //pomocnicza zmienna do liczenia ktory z kolei znak wczytujemy

    while ((odczytany_bajt = read(zadania, &znak, 1)) > 0) {
        if (znak == '\n') {
            linia[i] = '\0';
            i = 0;

            wartosc = strtok(linia, ":");
            zadania_tab[ilosc_zadan][0] = atoi(wartosc);

            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][1] = atoi(wartosc);

            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][2] = atoi(wartosc);
            strcpy(komendy[ilosc_zadan], wartosc);

            wartosc = strtok(NULL, ":");
            zadania_tab[ilosc_zadan][3] = atoi(wartosc);

            ilosc_zadan++;

            // realokacja pamięci, jeśli ilość zadań przekroczyła początkowy rozmiar tablicy
            if (ilosc_zadan >= n) {
                n += 100; // zwiększenie maksymalnej liczby zadań o 100
                zadania_tab = (int **)realloc(zadania_tab, n * sizeof(int *));
                for (int j = ilosc_zadan; j < n; j++) {
                    zadania_tab[j] = (int *)malloc(4 * sizeof(int));
                }

                komendy = (char **)realloc(komendy, n * sizeof(char *));
                for (int j = ilosc_zadan; j < n; j++) {
                    komendy[j] = (char *)malloc(100 * sizeof(char));
                }
            }
        } else {
            linia[i++] = znak;
        }
    }

    close(zadania); //Zamknięcie pliku taskfile po wczytaniu wszystkich zadań

    //Filtrowanie zadań (pozbywanie się zadań, które w pliku zostały wpisane z godziną wcześniejszą niż godzina rozpoczęcia programu)
    time_t now = time(NULL); //pobieramy czas z systemu
	struct tm *local = localtime(&now); //Zastosowanie funkcji localtime do konwertowania wartości now
	int godzina = local->tm_hour; //Pobranie wartości aktualnej godziny
	int minuta = local->tm_min; //Pobranie wartości aktualnej ilości minut

    int zadania_tab2[ilosc_zadan+1][4]; //Tablica zadań, która bedzie przechowywała przefiltrowane zadania
    char komendy2[ilosc_zadan+1][100]; //Tablica komend przechwująca przefiltrowane komendy
    int pomoc = 0;//Zmienna pomocnicza oznacza kolejne indeksy zadania_tab2


    for(int i = 0; i < ilosc_zadan ; i++) //Przechodzimy po wszystkich wczytanych zadaniach
    {
        //Jeśli czas wczytanego zadania jest późniejszy lub równy aktualnemu czasowi, to zadanie jest dodawane do tablicy zadania_tab2 na pozycji pomoc
        if(zadania_tab[i][0] > godzina || (zadania_tab[i][0] == godzina && zadania_tab[i][1] >= minuta))
        {
            zadania_tab2[pomoc][0] = zadania_tab[i][0]; //Przypisywanie kolejnych parametrów do nowej, przefiltrowanej tablicy
            zadania_tab2[pomoc][1] = zadania_tab[i][1];
            zadania_tab2[pomoc][2] = zadania_tab[i][2]; 
            strcpy(komendy2[pomoc], komendy[i]);
            zadania_tab2[pomoc][3] = zadania_tab[i][3];

            pomoc++; //Zwiększenie wartości pomocniczej zmiennej oznaczającej kolejne indeksy zadania_tab2
        }
    }
    //pomoc--; //Jednorazowe zmniejszenie wartości nowej ilości zadań

    //Alokacja pamięci tablicy tasks
    tasks = (int **) malloc(pomoc+1 * sizeof(int *));
    
    //Alokacja pamięci tablicy taski
    taski = (char *) malloc(pomoc+1 * sizeof(char));

    //wiersze = pomoc;

    //Przypisanie kolejnych wartości zadania_tab2 pod tasks
    for (int i = 0; i < pomoc; i++) 
    {
        tasks[i] = (int *) malloc(4 * sizeof(int));
        tasks[i] = zadania_tab2[i];
        taski[i] = komendy2[i];
    }

    //Sortowanie chronologiczne instrukcji (wczytujemy tablicę ze wszystkimi parametrami, ilość wszystkich wczytanych zadań oraz tablicę z konkretnymi komendami)
    Sortowanie(zadania_tab2, pomoc, komendy2);
    
    int zadanie = 0; //Oblicza ilość zrobionych zadań
    int kod_wyjscia = 0; //Pobiera kod wyjścia
    int sekundy = 0; //Pobiera za ile sekund musi się obudzić demon
    
    pid = fork(); //Funkcja fork uruchamia nowy proces potomny
    
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
    
    //SIGALRM inicjalizacja
    if (signal(SIGALRM, handler) == SIG_ERR)
    {
        perror("Nie udało się zarejestrować obsługi sygnału SIGALRM");
        exit(EXIT_FAILURE);
    }
    
    if (pid < 0) //Przypadek błędnego utworzenia procesu
    {
        printf("Nie udalo sie utworzyc procesu potomnego");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) 
    {
    	//Proces działa w pętli sprawdzając, czy juz jest czas na wykonanie zadania
    	while(pomoc >= zadanie)
    	{
            //Obliczenie ilości sekund pozostałych do wykonania kolejnego zadania
            sekundy = CzasDoZadania(zadania_tab2[zadanie][0], zadania_tab2[zadanie][1]);
            
            //Ustawienie czasomierza
            alarm(sekundy);
            
	    //printf("Demon obudzi sie za: %d sekund\n", sekundy);
	    pause(); //Demon śpi przez obliczoną ilość sekund, dzięki czemu budzi się o czasie wykonywania zadania określonym w taskfile

            pid_t pid2 = fork(); //Proces potomny wykonujący zadanie

            if (pid2 < 0) //Przypadek błędnego utworzenia procesu
	    	{
			    printf("Nie udalo sie utworzyc procesu potomnego dla polecenia %s.", komendy2[zadanie]);
			    exit(EXIT_FAILURE);
	    	}
            else if(pid2==0) //W przypadku sukcesu utworzenia procesu potomnego
            {
                //Wpisanie do logów i do pliku informacji o wykonywanym zadaniu
                syslog(LOG_INFO, "Uruchomiono polecenie %s z parametrem %d o godzinie %d:%d\n", komendy2[zadanie], zadania_tab2[zadanie][3], zadania_tab2[zadanie][0], zadania_tab2[zadanie][1]);
                char buf[100];
                snprintf(buf, sizeof(buf), "\nUruchomiono polecenie %s z parametrem %d o godzinie %d:%d\n\n", komendy2[zadanie], zadania_tab2[zadanie][3], zadania_tab2[zadanie][0], zadania_tab2[zadanie][1]);
                write(wypisanie, buf, strlen(buf));
                        
                //Warunki dotyczące kolejnych wartości parametru <info>
                if(zadania_tab2[zadanie][3] == 0)
                {
                    //Uruchamiana jest funkcja stdout_stderr z parametrem = 0, co oznacza, że ma wypisać wynik polecenia do pliku
                    stdout_stderr(wypisanie, komendy2, zadanie, 0);
                }
                else if(zadania_tab2[zadanie][3] == 1)
                {
                    //Uruchamiana jest funkcja stdout_stderr z parametrem = 1, co oznacza, że ma wypisać wynik wyjścia błędów do pliku (jeżeli jakiś błąd wystąpił)
                    stdout_stderr(wypisanie, komendy2, zadanie, 1);
                }
                else if(zadania_tab2[zadanie][3] == 2)
                {
                    //Uruchamiana jest funkcja stdout_stderr z parametrem = 2, co oznacza, że ma wypisać wynik polecenia i wynik wyjścia błędów do pliku
                    stdout_stderr(wypisanie, komendy2, zadanie, 2);
                }
                else
                {
                    //Przypadek podania niewłaściwego parametru <info>
                    snprintf(buf, sizeof(buf), "Nieprawidlowa wartosc parametru\n\n");
                    write(wypisanie, buf, strlen(buf));
                }
                    
                kod_wyjscia = close(wypisanie); //Odczytanie kodu wyjścia ze zmiennej otwierającej plik wyjściowy
                //Wpisanie do logów informacji o zakończonym zadaniu
                syslog(LOG_INFO, "Zakonczono polecenie %s z parametrem %d o godzinie %d:%d z kodem wyjscia %d\n", komendy2[zadanie],  zadania_tab2[zadanie][3], zadania_tab2[zadanie][0], zadania_tab2[zadanie][1], kod_wyjscia);
                exit(EXIT_SUCCESS);
            }
            zadanie++; //Zwiększamy zmienną zliczającą ilość wykonanych zadań
            if(zadanie==pomoc) //Jeżeli wykonaliśmy już ostatnie zadanie to kończymy proces potomny
            {
                exit(EXIT_SUCCESS);
            }
        }
    }
    exit(EXIT_SUCCESS);
}