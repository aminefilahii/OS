#ifndef GESCOM_H
#define GESCOM_H

// var globales pour stocker les mots de la commande
extern char **Mots;
extern int NMots;

int analyseCom(char *ligne);
void freeMots();
void majComInt(void);
void listeComInt(void);
int execComInt(int N, char **P);
int execComExt(char **P);

// fnct qui gere les pipes et toute l'execution
void traiterPipeline(char *ligne);

#endif // GESCOM_H
