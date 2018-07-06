/*--- Program byl pisany, aby byla mozliwosc jego skompilowania ---*/

/*---        oraz uruchomienia w terminale systemu linux        ---*/

/* Program mial na celu opracowanie asynchroniczego mechanizmu przekazywania */
/* informacji pomiedzy operatorem a procesami oraz pomiedzy procesami z wykorzystaniem*/
/* sygnalow, laczy nazwanych, semaforow oraz pamieci wspoldzielonej */
/* W dalszej czesci kodu jest opisane, co poszczegolny proces ma robic*/
/* oraz opisane zostalo jakie sygnaly mialy zostac uzyte w tym projekcie*/


#include <sys/types.h>

#include <sys/ipc.h>

#include <sys/sem.h>

#include <sys/shm.h>

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include <fcntl.h>

#include <unistd.h>

#include <signal.h>


/*----------------------- Zmienne globalne --------------------------*/

	int process_num; 

	int pids [2]; 

	int semid1; 
	
	int semid2; 

	int semid3; 

	int shmid; 
	
	char* shm; 
	
	int line_size = 200;


	/* Sygnaly i ich dzialanie */

	int S1 = SIGINT; // Zakonczenie dzialania procesow
	
	int S2 = SIGUSR1; // Zatrzymanie dzialania procesow
	
	int S3 = SIGCONT; // Przywrocenie dzialania procesow
	
	int S4 = SIGUSR2; // Powiadomienie pozostalych procesow o odebranym sygnale

/*-------------------------------------------------------------------------*/



/*--------------- Funkcja tworzaca semafor -----------------*/

	int sem_create(int keynb)
	{
	
		/*	Wygenerowanie klucza za pomoca 
	
			funkcji ftok, stworzenie semafora za
		
			pomoca funkcji semget z podanym kluczem
		
			i zwrocenie jego id   */
	
		
		key_t semkey = ftok(".", keynb);
	
		int semid = semget(semkey, 1, IPC_CREAT|0666);
	
		return semid;
	}
	
/*-----------------------------------------------------------*/	
	
	
/*----------------- Funkcja usuwajaca semafor ------------------*/	
	
	void sem_delete(int semid)
	{
		
		/*	Usuniecie semafora za pomoca funkcji semctl*/
		
		semctl(semid, 0, IPC_RMID);
		
	}
	
/*---------------------------------------------------------------*/	



/*-------------- Funkcja podnoszaca i funkcja opuszczajaca semafor -------------------*/

	void sem_up(int semid, int val)
	{
		
		/* 	Utworzenie obiektu operacji na semaforze
			
			i opsuzczenie semafora za pomoca funkcji 
			
			semop o wartosc val      */
		
		struct sembuf up = { 0, val, 0 };
		
		semop(semid, &up, 1);
	}
	
	
	void sem_down(int semid, int val)
	{
		
		/* 	Utworzenie obiektu operacji na semaforze
			
			i opsuzczenie semafora za pomoca funkcji 
			
			semop o wartosc -val      */
		
		struct sembuf down = { 0, -val, 0 };
		
		semop(semid, &down, 1);
	}

/*------------------------------------------------------------------------------------*/




/*--------------- Funkcja tworzaca pamiec wspoldzielona --------------------*/

	int shm_create(char **shm, int line_size)
	{
		
		/*  Wygenerowanie klucza za pomoca 
	
			funkcji ftok, stworzenie pamieci
			
			wspodzielonej za pomoca funkcji
			
			shmget z podanym kluczem, podlaczenie
		
			pamieci wspoldzielonej za pomoca funkcji
			
			shmat i zwrocenie jego id   */
			
	
		key_t shmkey = ftok(".", 200);
	
		int shmid = shmget(shmkey, line_size, IPC_CREAT|0666);
	
		*shm = shmat(shmid, NULL, 0);
	
		return shmid;
}

/*-----------------------------------------------------------------------*/



/*-------------------- Funkcja usuwajaca pamiec wspoldzielona ---------------------*/

	void shm_delete(char **shm, int shmid)
	{
		
		/*	Odlaczenie pamieci wspoldzielonej za pomoca
		
			funkcji shmdt, ustawienie wskaznika na pamiec
			
			wspoldzielona na NULL i usuniecie pamieci
			
			wspoldzelonej funkcja shmctl   */

		shmdt(shm);

		shm = NULL;

		shmctl(shmid, IPC_RMID, NULL);
	}

/*--------------------------------------------------------------------------------*/




/*------------- Funkcja czytajaca z pamieci wspoldzielonej -------------------------*/

	void shm_read(char* shm, int* data, size_t size)
	{
		/* 	Skopiowanie zawartosci pamieci wspoldzielonej
		
			do zmiennej data typu int    */
			
		memcpy(data, shm, size);
	}

/*------------------------------------------------------------------------------------*/




/*------------- Funkcja zapisujaca pamieci wspoldzielonej -------------------------*/

	void shm_write(char* shm, int* data, size_t size)
	{
		/* 	Skopiowanie zawartosci z zmiennej typu int
		
			do pamieci wspoldzielonej    */
			
		memcpy(shm, data, size);
		
	}

/*---------------------------------------------------------------------------------*/



/*------------- Funkcja konczaca dzialanie procesu ---------------*/

void killProcess()
{
	/* 	Wypisanie ktory proces bedzie konczy 
	
		swoje dzialanie i zakonczenie jego dzialania*/
		
	fprintf(stderr, "KILL %d\n", process_num);
	
	exit(0);
	
}

/*-------------------------------------------------------------------------------------*/




/*---------------- Funkcja zatrzymujaca dzialanie procesu ----------------*/

	void stopProcess()
	{
			
		/*	Wyswietlenie ktory proces bedzie zatrzymany,
		
			nastepnie tworzymy maske sygnalow i ja wypelniamy.
			
			Nastepnie usuwamy z maski nastepujace sygnaly:
			
			- S1 (konczacy dzialanie reszty procesow) 
			
			- S3 (wznawiajacy prace procesu)
			
			- S4 (wznawiajacy prace procesu)
			
			Uzycie funkcji wstrzymania sigsuspend, ktory bedzie czekal
			
			na sygnal S3 lub S4, aby moc wznowic dzialanie procesu.*/	
		
		fprintf(stderr, "STOP %d\n", process_num);
		
		sigset_t mask;
		
		sigfillset(&mask);
		
		sigdelset(&mask, S1);
		
		sigdelset(&mask, S3);
		
		sigdelset(&mask, S4);
		
		sigsuspend(&mask);
		
	}

/*--------------------------------------------------------------------------*/



/*------------ Funkcja wyswietlajaca, ktory proces bedzie wznawiany ---------------------*/

	void resumeProcess()
	{
		
		fprintf(stderr, "RESUME %d\n", process_num);
	
	}
	
/*---------------------------------------------------------------------------------------*/	



/*------------------ Funkcja okreslajaca po przeslaniu sygnalu, ktora funkcja ma sie wykonac ---------------------*/

	void sigAction(int signum)
	{
		
		/*  W zaleznosci od otrzymanego sygnalu ma zostac
		
			 wykonana ktoras z ponizszych funkcji     */
		
		if(signum == S1) killProcess();
		
		else if(signum == S2) stopProcess();
		
		else if(signum == S3) resumeProcess();
		
	}

/*-------------------------------------------------------------------------------------------------------------------*/

	
	
	
/*-------- Funkcja obslugi sygnalu przez proces ktory go otrzymal ---------*/	
	
	void handleSig(int signum)
	{

		/*	Powiadamiane sa pozostale procesy o sygnale
		
			wywolanie funkcji zapisu do pamieci wspoldzielonej
			
			(zapisywany jest sygnal, ktory zostal wyslany). 
			
			Podniesienie semaforow procesom, ktorym zostal
			
			wyslany sygnal S4, ktore pozniej odczytaja sygnal
			
			i wykonaja odpowiednia funkcje     */
		
		kill(pids[0], S4);
		
		kill(pids[1], S4);
	
		shm_write(shm, &signum, sizeof(int));
	
		if(process_num != 1)	sem_up(semid1, 1);
		
		if(process_num != 2)	sem_up(semid2, 1);
		
		if(process_num != 3)	sem_up(semid3, 1);
	
		sigAction(signum);
		
	}

/*-------------------------------------------------------------------------*/





/*----------- Funkcja obslugi sygnalu przez procesy, ktore go odebraly -------------------*/

	void sigshm(int signum)
	{
	
		/*	Nastepuje opuszczenie semaforow i czekanie, az
			
			proces, ktory odebral sygnal je podniesie.   
			
			Przeczytanie z pamieci wspoldzielonej numeru 
			
			sygnalu i wykonanie odpowiedniej funkcji */

		if(process_num == 1)	sem_down(semid1, 1);
		
		else if(process_num == 2)	sem_down(semid2, 1);
		
		else if(process_num == 3) 	sem_down(semid3, 1);
	
		int sig;
		
		shm_read(shm, &sig, sizeof(int));
		
		sigAction(sig);
	}

/*---------------------------------------------------------------------------------------------*/





/*------------------ Funkcja do wymiany identyfikatorow pomiedzy procesami ------------------- */

	void pid_exchange()
	{
		
		/*	Deskryptory dla laczy nazwanych
			
			z f1 i f2 czytaja pozostale procesy
			
			a z fifo czyta aktualny proces */
		
		int f1, f2, fifo;
			
		int pid = getpid();
		
			
		/*	Poszczegolne procesy czekaja otwieraja
			
			lacza nazwane, w celu przekazania swojego 
			
			identyfikatorow	pozostalym procesom 
			
			i odczytania identyfikatorow pozostalych 
			
			procesow  */
		
		if(process_num == 1)
		{
			
			fifo = open("fifopid1", O_RDONLY);
			
			f1 = open("fifopid2", O_WRONLY);
			
			f2 = open("fifopid3", O_WRONLY);
		
		}
		
		else if(process_num == 2)
		{
			
			f1 = open("fifopid1", O_WRONLY);
			
			fifo = open("fifopid2", O_RDONLY);
			
			f2 = open("fifopid3", O_WRONLY);
			
		}
		
		else if(process_num == 3)
		{
			
			f1 = open("fifopid1", O_WRONLY);
			
			f2 = open("fifopid2", O_WRONLY);
			
			fifo = open("fifopid3", O_RDONLY);
			
		}
		
		/*	Kazdy proces zapisuje swoj pid dwom pozostalym procesom,
		
			po zapisaniu deskryptory f1 i f2 sa zamykane i proces odczekuje
			
			10 ms. Po odczekaniu tego czasu sa odczytywane identyfikatory
			
			pozostalych procesow i deskryptor fifo zostaje zamkniety, 
			
			a tymczasowe lacza nazwane sa usuwane    */
		
		write(f1, &pid, sizeof(int));
		
		write(f2, &pid, sizeof(int));
		
		close(f1);
		
		close(f2);
		
		usleep(10000);
		
		read(fifo, &pids [0], sizeof(int));
		
		read(fifo, &pids [1], sizeof(int));
		
		close(fifo);
		
		if(process_num == 1)	unlink("fifopid1");
		
		if(process_num == 2)	unlink("fifopid2");
		
		if(process_num == 3)	unlink("fifopid3");
	}

/*-------------------------------------------------------------------------------------------*/





/*-------------- Funkcja procesu nr. 1 -------------------*/

	void proces1()
	{
		
		/* 	Otwierane jest lacze nazwane w trybie tylko do zapisu
		
			i tworzony bufor dla odczytanych danych z wejscia
			
			standardowego. Dane sa zapisywane do bufora, po czym
			
			laczem fifo12 sa przesylane do procesu nr. 2. Po
			
			przeslaniu danych lacze fifo12 jest zamykane  */
		
		int fifo12 = open("fifo12", O_WRONLY);
		
		char bufor [line_size];

		while(fgets(bufor, line_size, stdin) != NULL)
		{
			
			write(fifo12, bufor, strlen(bufor));
			
		}
		
		close(fifo12);
		
}

/*----------------------------------------------------------------/*






/*-------------- Funkcja procesu nr. 2 ------------------*/

	void proces2()
	{
		
		/*	Otwierane sa dwa lacza nazwane:
			
			- fifo12 w trybie do odczytu
			
			- fifo23 w trybie do zapisu
			
			Tworzony jest bufor do odczytu danych z lacza
			
			fifo12 i zmienna bytes_read do wyliczenia liczby
			
			odebranych znakow. Po sprawdzeniu za pomoca funkcji
			
			strlen ilosci odebranych znakow i jest dalej
			
			przesylane do procesu nr. 3 za pomoca lacza fifo23.
			
			Po tym nastepuje zamkniecie laczy fifo12 i fifo23 */
		
		FILE * fifo12 = fopen("fifo12", "r");
		
		int fifo23 = open("fifo23", O_WRONLY);
	
		char bufor [line_size];
		
		unsigned long bytes_read;
		
		while(fgets(bufor, line_size, fifo12) != NULL)
		{
			
			bytes_read = strlen(bufor);
			
			write(fifo23, &bytes_read, sizeof(unsigned long));
			
		}
		
		fclose(fifo12);
		
		close(fifo23);
		
	}

/*-----------------------------------------------------------------------*/


/*----------- Funkcja procesu nr. 3 ----------------*/

	void proces3()
	{
		
		/*	Stworzenie zmiennych pomocniczych,
		
			otwarcie lacza fifo23 w trybie do odczytu.
			
			Nastepuje pobranie danych z procesu nr. 2
			
			ktore pozniej sa wypisywane przez proces nr. 3.
			
			Po wszystkim nastepuje zamkniecie lacza fifo23 */
			
		ssize_t n;
		
		ssize_t bytes_read;
		
		int fifo23 = open("fifo23", O_RDONLY);
		
		while((n = read(fifo23, &bytes_read, sizeof(bytes_read))) > 0)
		{
			
			printf("Ilosc znakow w linii: %lu\n", bytes_read);
			
		}
		
		close(fifo23);
	}

/*-----------------------------------------------------------*/





/*------------------------ Ogolna funkcja procesow --------------------------------------*/
	
	void proces(int i)
	{
		
		/*	Wykonywana jest funkcja wymiany identyfikatorow miedzy procesami
			
			Na podstawie zadanego dla funkcji wartosci i wybierana jest
			
			odpowiednia funkcja dla procesu   */
		
		pid_exchange();
		
		/*- Obsluga sygnalow -*/
		
		signal(S1, handleSig);
		
		signal(S2, handleSig);
		
		signal(S3, handleSig);
		
		signal(S4, sigshm);

		/*-------------------*/
	
		if(i == 1)	proces1();
		
		else if(i == 2)	proces2();
		
		else if(i == 3)	proces3();

		exit(0);
	}
	
/*---------------------------------------------------------------------------------------*/



/*----------------------- Glowna funkcja programu --------------------------------*/

	int main()
	{
		
		/*	Tworzone sa lacza nazwane pomiedzy procesami 1->2 i 2->3,
		
			tymczasowe lacza do wymiany identyfikatorow miedzy procesami
			
			za pomoca funkcji mkfifo, semafory, oraz pamiec wspoldzielona
			
			o wielkosci inta (domyslnie 48). Nastepnie za pomoca funkcji fork
			 
			tworzone sa 3 procesy. Po wyslaniu sygnalu konca procesow
			
			pamiec jest zwalniana poprzez usuwanie stworzonych laczy,
			
			semaforow i pamieci wspoldzielonej     */
		
		
		mkfifo("fifo12", 0666);
		
		mkfifo("fifo23", 0666);

		mkfifo("fifopid1", 0666);
		
		mkfifo("fifopid2", 0666);
		
		mkfifo("fifopid3", 0666);


		semid1 = sem_create(0);
		
		semid2 = sem_create(1);
		
		semid3 = sem_create(2);
		
		
		shmid = shm_create(&shm, sizeof(int));
	
		
		if(fork() == 0)	proces(1);
		
		else if(fork() == 0)	proces(2);
		
		else if(fork() == 0)	proces(3);

		
		wait(NULL);
		
		wait(NULL);
		
		wait(NULL);

		
		unlink("fifo12"); 
		
		unlink("fifo23"); 
		
		sem_delete(semid1); 
		
		sem_delete(semid2); 
		
		sem_delete(semid3); 
		
		shm_delete(&shm, shmid); 
		
		return 0;
	}
	
/*---------------------------------------------------------------------------*/
