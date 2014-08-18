/******************************************************
* licence:Cette oeuvre, création, site ou texte est sous 
* licence Creative Commons Attribution -Partage dans 
* les Mêmes Conditions 4.0 International. 
* Pour accéder à une copie de cette licence, 
* merci de vous rendre à l'adresse suivante 
* http://creativecommons.org/licenses/by-sa/4.0/ 
* ou envoyez un courrier à Creative Commons, 444 Castro Street,
* Suite 900, Mountain View, California, 94041, USA.
* ****************************************************
* Auteur: Van der Linden Nicolas
* Website: http://flashnation.net
* GitHub: https://github.com/nicovan/Tetris-ZeroGravity
* Application: Tetris zero gravity
* Date: 01/03/2014
******************************************************/


// <author>Barry Dorrans</author>
// <email>barryd@idunno.org</email>
// <date>2008-06-10</date>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"
#include "ClientTetris.h"

// Dimensions de la grille de jeu
#define NB_LIGNES   14
#define NB_COLONNES 20

// Nombre de cases maximum par piece
#define NB_CASES    4

// Macros utlisees dans le tableau tab
#define VIDE        0

void setMessage(const char *texte);
void* threadDefileMessage(void*);
void* threadPiece(void*);
void* threadEvent(void*);
void* threadScore(void*);
void* threadCases(void*);
void* threadGravite(void*);
void handlerSIGUSR1 (int);
void* threadFinPartie(void*);
void TriTableau(int * tabInt,int taille);

int tab[NB_LIGNES][NB_COLONNES]
={ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
  
typedef struct
{
  int ligne;
  int colonne;
} CASE;

typedef struct
{
  CASE cases[NB_CASES];
  int  nbCases;
  int  professeur;
} PIECE;

PIECE pieces[7] = { 0,0,0,1,1,0,1,1,4,WAGNER,       // carre
                    0,0,1,0,2,0,2,1,4,MERCENIER,    // L
                    0,1,1,1,2,0,2,1,4,VILVENS,      // J
                    0,0,0,1,1,1,1,2,4,DEFOOZ,       // Z
                    0,1,0,2,1,0,1,1,4,GERARD,       // S
                    0,0,0,1,0,2,1,1,4,CHARLET,      // T
                    0,0,0,1,0,2,0,3,4,MADANI };     // I

void RotationPiece(PIECE*);
char* message=NULL;//pointeur vers le message à faire défiler
int tailleMessage;//longueur du message
int indiceCourant;//indice du premier caractère à afficher dans la zone graphique
CASE casesInserees[NB_CASES]; //cases insérées par le joueur.
int nbCasesInserees=0; //nombre de cases actuellement insérées par le joueur.
PIECE pieceEnCours;
char MAJScore=0;
int score=0;
CASE *pCases;
int lignesCompletes[4];
int nbLignesCompletes=0;
int colonnesCompletes[4];
int nbColonnesCompletes=0;
int nbAnalyses;

struct sigaction sigAct;

pthread_mutex_t mutexCasesInserees;
pthread_cond_t condCasesInserees;
pthread_mutex_t mutexMessage;
pthread_mutex_t mutexScore;
pthread_cond_t condScore;
pthread_key_t cle;
pthread_mutex_t mutexAnalyse;
pthread_mutex_t mutexCaseSet;
pthread_cond_t condAnalyse;

pthread_t tabThreadCases[14][10];
pthread_t threadHandleFinPartie;

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
  EVENT_GRILLE_SDL event;
	int i,j;
  char buffer[80];
  char ok;
	pthread_t threadHandleDefileMessage;
	pthread_t threadHandlePiece;
	pthread_t threadHandleEvent;
	pthread_t threadHandleScore;
	pthread_t threadHandleGravite;
 
  srand((unsigned)time(NULL));
	
	sigemptyset(&sigAct.sa_mask);
	sigAct.sa_handler = handlerSIGUSR1;
	sigaction(SIGUSR1,&sigAct,NULL);
	
	sigemptyset(&sigAct.sa_mask);
	sigaction(SIGUSR2,&sigAct,NULL);

  // Ouverture de la grille de jeu (SDL)
  printf("(THREAD MAIN) Ouverture de la grille de jeu\n");
  fflush(stdout);
  sprintf(buffer,"!!! TETRIS ZERO GRAVITY !!!");
  if (OuvrirGrilleSDL(NB_LIGNES,NB_COLONNES,40,buffer) < 0)
  {
    printf("Erreur de OuvrirGrilleSDL\n");
    fflush(stdout);
    exit(1);
  }

  // Chargement des sprites et de l'image de fond
  ChargementImages();
  DessineSprite(12,11,VOYANT_VERT);

	pthread_mutex_init(&mutexMessage,NULL);
	pthread_mutex_init(&mutexCasesInserees,NULL);
	pthread_cond_init(&condCasesInserees,NULL);

	//Creation du thread threadDefileMessage
	if(pthread_create(&threadHandleDefileMessage,NULL,(void*(*)(void*))threadDefileMessage,(void*)NULL)!=0)
	{
		printf("(THREAD MAIN) Erreur de creation threadDefileMessage\n");
		exit(-1);
	}
	//Creation thread piece
	if(pthread_create(&threadHandlePiece,NULL,(void*(*)(void*))threadPiece,(void*)NULL)!=0)
	{
		printf("(THREAD MAIN) Erreur de creation threadPiece\n");
		exit(-1);
	}
	//Creation du thread event
	if(pthread_create(&threadHandleEvent,NULL,(void*(*)(void*))threadEvent,(void*)NULL)!=0)
	{
		printf("(THREAD MAIN) Erreur de creation threadEvent\n");
		exit(-1);
	}
	//Creation du thread Score
	if(pthread_create(&threadHandleScore,NULL,(void*(*)(void*))threadScore,(void*)NULL)!=0)
	{
		printf("(THREAD MAIN) Erreur de creation threadScore\n");
		exit(-1);
	}
	//Creation des threads Cases
	pthread_key_create(&cle,NULL);
	for(i=0;i<14;i++)
	{
		for(j=0;j<10;j++)
		{
			pthread_mutex_lock(&mutexCaseSet);
			if(!pCases)
				pCases=(CASE*)malloc(sizeof(CASE));
			pCases->ligne=i;
			pCases->colonne=j;
			if(pthread_create(&tabThreadCases[i][j],NULL,(void*(*)(void*))threadCases,(void*)pCases)!=0)
			{
				printf("(THREAD MAIN) Erreur de creation threadCases\n");
				exit(-1);
			}
		}
	}
	//Creation threadGravite
	if(pthread_create(&threadHandleGravite,NULL,(void*(*)(void*))threadGravite,(void*)NULL)!=0)
	{
		printf("(THREAD MAIN) Erreur de creation threadGravite\n");
		exit(-1);
	}
	//Creation threadFinPartie
	if(pthread_create(&threadHandleFinPartie,NULL,(void*(*)(void*))threadFinPartie,(void*)NULL)!=0)
	{
		printf("(THREAD MAIN) Erreur de creation threadFinPartie\n");
		exit(-1);
	}

	setMessage("Bienvenue dans Tetris Zero Gravity");
  ok = 0;
  while(!ok)
  {
    event = ReadEvent();
    if (event.type == CROIX) ok = 1;
  }

	//pthread_join(threadHandleFinPartie,NULL);

  // Fermeture de la grille de jeu (SDL)
  printf("(THREAD MAIN) Fermeture de la grille..."); fflush(stdout);
  FermerGrilleSDL();
	pthread_cancel(threadHandleDefileMessage);
	pthread_cancel(threadHandlePiece);
	pthread_cancel(threadHandleEvent);
	pthread_cancel(threadHandleScore);
	for(i=0;i<14;i++)
	{
		for(j=0;j<10;j++)
		{
			pthread_cancel(tabThreadCases[i][j]);
		}
	}
	pthread_cancel(threadHandleGravite);
	pthread_cancel(threadHandleFinPartie);
  printf("OK\n"); fflush(stdout);

  exit(0);
}
void* threadDefileMessage(void*)
{
	int i,j;
	struct timespec temps;
	temps.tv_sec=0;
	temps.tv_nsec=400000000;
	while(1)
	{
		j=indiceCourant;
		for(i=11;i<=18;i++)
		{
			if(j>=tailleMessage)
			{
				DessineLettre(10,i,' ');
			}
			else
			{
				if((message[j]>='a' && message[j]<='z')||(message[j]>='A' && message[j]<='Z')||message[j]==' ')
				{
					DessineLettre(10,i,message[j]);
				}
				else
				{
					if(message[j]>=48 && message[j]<=57)
						{DessineChiffre(10,i,message[j]-48);}
					else
					{DessineLettre(10,i,' ');}
				}
			}
			j++;
		}
		nanosleep(&temps,NULL);
		if(indiceCourant==tailleMessage)
			indiceCourant=-1;
		indiceCourant++;
	}
}
void setMessage(const char *texte)
{
	pthread_mutex_lock(&mutexMessage);
	if(message)
		free(message);
	tailleMessage=strlen(texte);
	message=(char*)malloc(tailleMessage);
	strcpy(message,texte);
	indiceCourant=0;
	pthread_mutex_unlock(&mutexMessage);
}

void* threadPiece(void*)
{
	int choixPiece,i,j,tour,Lmin,Cmin,trouve;
	srand(time(NULL));
	trouve=1;
	while(1)
	{
		if(trouve==1)
		{
			/****************************************************/
			memcpy(&pieceEnCours,&pieces[/*rand()%7*/6],sizeof(PIECE));
			tour=(rand()%4);
			tour=1;
			for(i=0;i<tour;i++)
			{
				RotationPiece(&pieceEnCours);	//Rotation de la piece entre 0 et 3 fois
			}
			for(i=3;i<=6;i++)
			{
				for(j=15;j<=18;j++)
				{
					EffaceCarre(i,j);
				}
			}
			for(i=0;i<NB_CASES;i++)	
			{
				DessineSprite(pieceEnCours.cases[i].ligne+3,pieceEnCours.cases[i].colonne+15,pieceEnCours.professeur);
			}
		}
		pthread_mutex_lock(&mutexCasesInserees);
		while(nbCasesInserees<pieceEnCours.nbCases)
			pthread_cond_wait(&condCasesInserees,&mutexCasesInserees);
		pthread_mutex_unlock(&mutexCasesInserees);
		//verification piece
		Lmin=15;Cmin=11;
		for(i=0;i<pieceEnCours.nbCases;i++)
		{
			if(casesInserees[i].ligne<Lmin)
				Lmin=casesInserees[i].ligne;
			if(casesInserees[i].colonne<Cmin)
				Cmin=casesInserees[i].colonne;
		}
		//comparer pieces
		trouve=1;
		for(j=0;j<pieceEnCours.nbCases && trouve !=0;j++)
		{
			trouve=0;
			for(i=0;i<pieceEnCours.nbCases && trouve!=1;i++)
			{
				if(casesInserees[j].ligne-Lmin==pieceEnCours.cases[i].ligne &&	casesInserees[j].colonne-Cmin==pieceEnCours.cases[i].colonne)
					trouve=1;
			}
		}
		if(trouve==1) //trouvé
		{
			for(i=0;i<pieceEnCours.nbCases;i++)
			{
				DessineSprite(casesInserees[i].ligne,casesInserees[i].colonne,BRIQUE);
				tab[casesInserees[i].ligne][casesInserees[i].colonne]=BRIQUE;
				pthread_kill(tabThreadCases[casesInserees[i].ligne][casesInserees[i].colonne],SIGUSR1);
			}
			pthread_mutex_lock(&mutexCasesInserees);
			nbCasesInserees=0;
			pthread_mutex_unlock(&mutexCasesInserees);
			pthread_mutex_lock(&mutexScore);
				score+=1;
				MAJScore=1;
			pthread_mutex_unlock(&mutexScore);
			pthread_cond_signal(&condScore);
		}
		else	//non trouvé
		{
			for(i=0;i<pieceEnCours.nbCases;i++)
			{
				EffaceCarre(casesInserees[i].ligne,casesInserees[i].colonne);
				tab[casesInserees[i].ligne][casesInserees[i].colonne]=0;
			}
			pthread_mutex_lock(&mutexCasesInserees);
			nbCasesInserees=0;
			pthread_mutex_unlock(&mutexCasesInserees);
		}
	}
}

void RotationPiece(PIECE *pPiece)
{
	int i,Lmin=0,Cmin=0,tmp;
	
	for(i=0;i<pPiece->nbCases;i++)
	{
		tmp=pPiece->cases[i].ligne;
		pPiece->cases[i].ligne=-(pPiece->cases[i].colonne);
		pPiece->cases[i].colonne=tmp;
		if(pPiece->cases[i].ligne<Lmin)
			Lmin=pPiece->cases[i].ligne;
		if(pPiece->cases[i].colonne<Cmin)
			Cmin=pPiece->cases[i].colonne<Cmin;
	}
	if(Lmin<0)
	{
		for(i=0;i<NB_CASES;i++)
		{
			pPiece->cases[i].ligne+=-Lmin;
		}
	}
	if(Cmin<0)
	{
		for(i=0;i<NB_CASES;i++)
		{
			pPiece->cases[i].colonne+=-Cmin;
		}
	}
}

void* threadEvent(void*)
{
	char ok = 0;
	EVENT_GRILLE_SDL event;
  while(!ok)
  {
    event = ReadEvent();
    if (event.type == CROIX) ok = 1;
    if (event.type == CLIC_GAUCHE && event.colonne<10 && tab[event.ligne][event.colonne]==0)
		{
			DessineSprite(event.ligne,event.colonne,pieceEnCours.professeur);
			tab[event.ligne][event.colonne]=pieceEnCours.professeur;
			casesInserees[nbCasesInserees].ligne=event.ligne;
			casesInserees[nbCasesInserees].colonne=event.colonne;
			pthread_mutex_lock(&mutexCasesInserees);
			nbCasesInserees++;
			pthread_mutex_unlock(&mutexCasesInserees);
			pthread_cond_signal(&condCasesInserees);
		}
		if (event.type == CLIC_DROIT && nbCasesInserees>0)
		{
			pthread_mutex_lock(&mutexCasesInserees);
			nbCasesInserees--;
			pthread_mutex_unlock(&mutexCasesInserees);
			while(nbCasesInserees!=-1)
			{
				EffaceCarre(casesInserees[nbCasesInserees].ligne,casesInserees[nbCasesInserees].colonne);
				tab[casesInserees[nbCasesInserees].ligne][casesInserees[nbCasesInserees].colonne]=0;
				pthread_mutex_lock(&mutexCasesInserees);
				nbCasesInserees--;
				pthread_mutex_unlock(&mutexCasesInserees);
			}
			pthread_mutex_lock(&mutexCasesInserees);
			nbCasesInserees=0;
			pthread_mutex_unlock(&mutexCasesInserees);
		}
  }
}

void* threadScore(void*)
{
	int tmp;
	while(1)
	{
		pthread_mutex_lock(&mutexScore);
			while(!MAJScore)
				pthread_cond_wait(&condScore,&mutexScore);
		
		DessineChiffre(1,18,score%10);
		DessineChiffre(1,17,score/10%10);
		DessineChiffre(1,16,score/100%10);
		DessineChiffre(1,15,score/1000%10);
		MAJScore=0;
		pthread_mutex_unlock(&mutexScore);
	}
}

void* threadCases(void* pCases)
{
	CASE *caseGeree;
	caseGeree=(CASE*)malloc(sizeof(CASE));
	caseGeree->ligne=((CASE*)pCases)->ligne;
	caseGeree->colonne=((CASE*)pCases)->colonne;
	pthread_mutex_unlock(&mutexCaseSet);
	pthread_setspecific(cle,caseGeree);
	while(1)
	{
		pause();
	}
}

void handlerSIGUSR1 (int sig)
{
	int nbCaseTemp,cont,i;
	CASE *caseTemp;
	pthread_mutex_lock(&mutexAnalyse);

	caseTemp=(CASE*)pthread_getspecific(cle);
	//Analyse Ligne
	for(i=0,nbCaseTemp=0;i<10;i++)//verifie si ligne complete
	{
		if(tab[caseTemp->ligne][i]==BRIQUE)
			nbCaseTemp++;
	}
	if(nbCaseTemp==i)
	{
		for(i=0,cont=1;i<nbLignesCompletes;i++)//verifie si deja pas trouve par un autre thread
		{
			if(lignesCompletes[i]==caseTemp->ligne)
			{
				cont=0;
				break;
			}
		}
		if(cont==1)
		{
			nbLignesCompletes++;
			lignesCompletes[i]=caseTemp->ligne;
			for(i=0;i<10;i++)
			{
				DessineSprite(caseTemp->ligne,i,FUSION);
			}
		}
	}
	//Analyse Colonne
	for(i=0,nbCaseTemp=0;i<14;i++)
	{
		if(tab[i][caseTemp->colonne]==BRIQUE)
			nbCaseTemp++;
	}
	if(nbCaseTemp==i)
	{
		for(i=0,cont=1;i<nbColonnesCompletes;i++)
		{
			if(colonnesCompletes[i]==caseTemp->colonne)
			{
				cont=0;
				break;
			}
		}
		if(cont==1)
		{
			nbColonnesCompletes++;
			colonnesCompletes[i]=caseTemp->colonne;
			for(i=0;i<14;i++)
			{
				DessineSprite(i,caseTemp->colonne,FUSION);
			}
		}
	}
	nbAnalyses++;
	pthread_mutex_unlock(&mutexAnalyse);
	pthread_cond_signal(&condAnalyse);
	
}

void* threadGravite(void*)
{
	int i,j,tmp;
	struct timespec temps;
	temps.tv_sec=0;
	temps.tv_nsec=500000000;

	while(1)
	{
		pthread_mutex_lock(&mutexAnalyse);
		while(nbAnalyses<pieceEnCours.nbCases)
			pthread_cond_wait(&condAnalyse,&mutexAnalyse);
		printf("%d%d\n",nbLignesCompletes,nbColonnesCompletes);
		if(nbLignesCompletes == 0 && nbColonnesCompletes == 0)
		{
			nbAnalyses=0;
		}
		else
		{
			printf("ICICICICI\n");
			TriTableau(colonnesCompletes,nbColonnesCompletes);
			TriTableau(lignesCompletes,nbLignesCompletes);
			printf("OKOKOKOK\n");
			printf("colonnes:\n");
			for(i=0;i<nbColonnesCompletes;i++)
			{printf("c: %d\n",colonnesCompletes[i]);}
			printf("Lignes:\n");
			for(i=0;i<nbLignesCompletes;i++)
			{printf("l: %d\n",lignesCompletes[i]);}
			for(i=0;i<nbColonnesCompletes && colonnesCompletes[i]<5;i++)//Recule colonne <5
			{
				nanosleep(&temps,NULL);

					tmp=colonnesCompletes[i];
					while(tmp!=0)
					{
						for(j=0;j<14;j++)
						{
							tab[j][tmp]=tab[j][tmp-1];
							if(tab[j][tmp]==0)
							{
								EffaceCarre(j,tmp);
							}
							else
							{
								DessineSprite(j,tmp,BRIQUE);
							}
						}
						tmp--;
					}
					for(j=0;j<14;j++)
					{
						EffaceCarre(j,0);
						tab[j][0]=0;
					}
					//AJOUT 5 POINTS
					pthread_mutex_unlock(&mutexScore);
					score+=5;
					MAJScore++;
					pthread_mutex_unlock(&mutexScore);
					pthread_cond_signal(&condScore);
				}
				for(i=nbColonnesCompletes-1;i!=-1 && colonnesCompletes[i]>=5;i--)	//Recule Colonne >=5
				{
					nanosleep(&temps,NULL);
					tmp=colonnesCompletes[i];
					while(tmp!=10)
					{
						for(j=0;j<14;j++)
						{
							tab[j][tmp]=tab[j][tmp+1];
							if(tab[j][tmp]==0)
							{
								EffaceCarre(j,tmp);
							}
							else
							{
								DessineSprite(j,tmp,BRIQUE);
							}
						}
						tmp++;
					}
					for(j=0;j<14;j++)
					{
						EffaceCarre(j,9);
						tab[j][9]=0;
					}
					//AJOUT 5 POINTS
					pthread_mutex_unlock(&mutexScore);
					score+=5;
					MAJScore++;
					pthread_mutex_unlock(&mutexScore);
					pthread_cond_signal(&condScore);
				}
			for(i=0;i<nbLignesCompletes && lignesCompletes[i]<7;i++)//Recule Ligne<7
			{
				nanosleep(&temps,NULL);

					tmp=lignesCompletes[i];
					while(tmp!=0)
					{
						for(j=0;j<10;j++)
						{
							tab[tmp][j]=tab[tmp-1][j];
							if(tab[tmp][j]==0)
							{
								EffaceCarre(tmp,j);
							}
							else
							{
								DessineSprite(tmp,j,BRIQUE);
							}
						}
						tmp--;
					}
					for(j=0;j<10;j++)
					{
						EffaceCarre(0,j);
						tab[0][j]=0;
					}
					//AJOUT 5 POINTS
					pthread_mutex_unlock(&mutexScore);
					score+=5;
					MAJScore++;
					pthread_mutex_unlock(&mutexScore);
					pthread_cond_signal(&condScore);
				}
				for(i=nbLignesCompletes-1;i!=-1 && lignesCompletes[i]>=7;i--)//Recule Ligne>=7
				{
					nanosleep(&temps,NULL);
					tmp=lignesCompletes[i];
					while(tmp!=14)
					{
						for(j=0;j<10;j++)
						{
							tab[tmp][j]=tab[tmp+1][j];
							if(tab[tmp][j]==0)
							{
								EffaceCarre(tmp,j);
							}
							else
							{
								DessineSprite(tmp,j,BRIQUE);
							}
						}
						tmp++;
					}
					for(j=0;j<10;j++)
					{
						EffaceCarre(13,j);
						tab[13][j]=0;
					}
					//AJOUT 5 POINTS
					pthread_mutex_unlock(&mutexScore);
					score+=5;
					MAJScore++;
					pthread_mutex_unlock(&mutexScore);
					pthread_cond_signal(&condScore);
				}
			nbAnalyses=0;
			nbColonnesCompletes=0;
			nbLignesCompletes=0;
		}
		pthread_mutex_unlock(&mutexAnalyse);
		//pthread_kill(threadHandleFinPartie,SIGUSR2);
	}
}

void* threadFinPartie(void*)
{
	int i,j,x,l,c,cont;
	while(1)
	{
		pause();
		cont=0;
		for(i=0;i<14 && cont==0;i++)
		{
			for(j=0;j<10 && cont==0;j++)
			{
				l=i;c=j;cont=1;
				for(x=0;x<pieceEnCours.nbCases-1 && cont==1;x++)
				{
					if(tab[l][c]!=0)
					{
						cont=0;
					}
					else
					{
						l+=pieceEnCours.cases[x+1].ligne-pieceEnCours.cases[x].ligne;
						c+=pieceEnCours.cases[x+1].colonne-pieceEnCours.cases[x].colonne;
					}
				}
			}
		}
		if(cont==0)
			pthread_exit(NULL);
	}
}

void TriTableau(int * tabInt,int taille)
{
	int i, j, indMin, Min;

	for(i = 0;i<taille;i++)
	{
		indMin=i;
		//indice MIN
		for(j=i;j<taille;j++)
		{
			if(tabInt[j]<tabInt[indMin])
				indMin=j;
			Min=tabInt[indMin];
		}
		//Décalage
		for(j=indMin;j>i;j--)
		{
			tabInt[j]=tabInt[j-1];
		}
		tabInt[i]=Min;
	}
}
