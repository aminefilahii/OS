#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>
#include "gescom.h"

#define NBMAXC 10

// var globales pour stocker les mots de la commande
char **Mots = NULL;
int NMots = 0;

// structure pour les commandes internes
typedef struct {
    char *nom;
    int (*fonc)(int, char **);
} CommandeInterne;

// table des commandes internes
static CommandeInterne TabCom[NBMAXC];
static int NComInt = 0;

// commande interne exit
static int Sortie(int N, char **P)
{
    (void)N;
    (void)P;
    exit(EXIT_SUCCESS);
}

// commande interne cd
static int Cd(int N, char **P)
{
    if (N > 1) {
        if (chdir(P[1]) != 0) {
            perror("cd");
        }
    } else {
        if (chdir(getenv("HOME")) != 0) {
            perror("cd");
        }
    }
    return 1;
}

// commande interne pwd
static int Pwd(int N, char **P)
{
    (void)N; 
    (void)P; 
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
    return 1;
}

// commande interne vers
static int Vers(int N, char **P)
{
    (void)N;
    (void)P;
    printf("biceps version 1.2\n");
    return 1;
}

// ajoute une commande interne dans la table
static void ajouteCom(char *nom, int (*fonc)(int, char **))
{
    if (NComInt >= NBMAXC) {
        fprintf(stderr, "erreur : trop de commandes internes\n");
        exit(EXIT_FAILURE);
    }
    TabCom[NComInt].nom = nom;
    TabCom[NComInt].fonc = fonc;
    NComInt++;
}

// mise a jour des commandes internes
void majComInt(void)
{
    ajouteCom("exit", Sortie);
    ajouteCom("cd", Cd);
    ajouteCom("pwd", Pwd);
    ajouteCom("vers", Vers);
}

// pour afficher les commandes internes
void listeComInt(void)
{
    printf("commandes internes :\n");
    for (int i = 0; i < NComInt; i++) {
        printf("%s\n", TabCom[i].nom);
    }
}

// fnct qui analyse la commande et la decoupe en mots
int analyseCom(char *ligne)
{
    char *tmp = strdup(ligne); // on fait une copie car strsep modifie la chaine
    char *save = tmp; // on garde l'adresse de debut pour free
    char *token;
    int i = 0;
    
    Mots = malloc(100 * sizeof(char *)); // table de mots (max 100)
    if (Mots == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    token = strsep(&tmp, " \t\n");
    while (token != NULL) {
        if (strlen(token) > 0) { // on ignore les mots vides
            Mots[i++] = strdup(token); 
        }
        token = strsep(&tmp, " \t\n");
    }
    Mots[i] = NULL; // important pour execvp
    NMots = i;
    free(save); // liberation de la copie
    return NMots;
}

// fnct pour liberer la memoire des mots
void freeMots()
{
    for (int i = 0; i < NMots; i++) {
        free(Mots[i]);
    }
    free(Mots);
}

// execute une commande interne si elle existe, regarde le premier mot P[0], le compare avec les noms des commandes internes et si elle trouve une correspondance elle appelle la fonction associée puis retourne 1 sinon, elle retourne 0 => exécuter une commande externe
int execComInt(int N, char **P)
{
    if (N <= 0) {
        return 0;
    }
    for (int i = 0; i < NComInt; i++) {
        if (strcmp(P[0], TabCom[i].nom) == 0) {
            TabCom[i].fonc(N, P);
            return 1;
        }
    }
    return 0;
}

// execute une commande externe, fork pour créer un processus fils puis le fils exécute la commande, le processus père attend que le fils termine. Si une erreur se produit elle affiche un message d’erreur
int execComExt(char **P)
{
    pid_t pid;
    int status;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 0;
    }
    if (pid == 0) { // fils
        execvp(P[0], P);
        perror(P[0]); 
        exit(EXIT_FAILURE);
    } else { // pere
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            return 0;
        }
    }
    return 1;
}

// fnct pour decaler les mots et supprimer une redirection du tableau
static void supprimerRedirection(int index, int nb_elements)
{
    for (int j = 0; j < nb_elements; j++) {
        free(Mots[index + j]); // liberation du symbole et du fichier
    }
    for (int j = index; j <= NMots - nb_elements; j++) {
        Mots[j] = Mots[j + nb_elements];
    }
    NMots -= nb_elements;
}

// fnct pour gerer les redirections avant execution
static void gererRedirections()
{
    int i = 0;
    while (i < NMots && Mots[i] != NULL) {
        if (strcmp(Mots[i], "<") == 0) {
            int fd = open(Mots[i+1], O_RDONLY);
            if (fd < 0) { perror(Mots[i+1]); exit(EXIT_FAILURE); }
            dup2(fd, STDIN_FILENO);
            close(fd);
            supprimerRedirection(i, 2);
        } else if (strcmp(Mots[i], ">") == 0) {
            int fd = open(Mots[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror(Mots[i+1]); exit(EXIT_FAILURE); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            supprimerRedirection(i, 2);
        } else if (strcmp(Mots[i], ">>") == 0) {
            int fd = open(Mots[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { perror(Mots[i+1]); exit(EXIT_FAILURE); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            supprimerRedirection(i, 2);
        } else if (strcmp(Mots[i], "2>") == 0) {
            int fd = open(Mots[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror(Mots[i+1]); exit(EXIT_FAILURE); }
            dup2(fd, STDERR_FILENO);
            close(fd);
            supprimerRedirection(i, 2);
        } else if (strcmp(Mots[i], "2>>") == 0) {
            int fd = open(Mots[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { perror(Mots[i+1]); exit(EXIT_FAILURE); }
            dup2(fd, STDERR_FILENO);
            close(fd);
            supprimerRedirection(i, 2);
        } else if (strcmp(Mots[i], "<<") == 0) {
            char *delim = Mots[i+1];
            int p[2];
            if (pipe(p) < 0) { perror("pipe"); exit(EXIT_FAILURE); }
            
            // on ouvre temporairement le flux tty pour lire du terminal
            FILE *tty = fopen("/dev/tty", "r");
            if (tty) {
                char buf[1024];
                while (fgets(buf, sizeof(buf), tty) != NULL) {
                    if (strncmp(buf, delim, strlen(delim)) == 0 && buf[strlen(delim)] == '\n') break;
                    write(p[1], buf, strlen(buf));
                }
                fclose(tty);
            }
            close(p[1]);
            dup2(p[0], STDIN_FILENO);
            close(p[0]);
            supprimerRedirection(i, 2);
        } else {
            i++;
        }
    }
}

// fnct qui gere les pipes et toute l'execution
void traiterPipeline(char *ligne)
{
    char *tmp = strdup(ligne); // on fait une copie pour strsep
    char *save_tmp = tmp;
    char *cmd;
    char *cmd_array[20];
    int nb_cmds = 0;
    
    // on decoupe par le delimiteur |
    while ((cmd = strsep(&tmp, "|")) != NULL && nb_cmds < 20) {
        if (strlen(cmd) > 0) {
            cmd_array[nb_cmds++] = strdup(cmd);
        }
    }
    
    // si une seule commande sans pipe
    if (nb_cmds == 1) {
        analyseCom(cmd_array[0]);
        if (NMots > 0) {
            // on verifie d'abord si c'est une commande interne (cd, pwd, exit)
            if (!execComInt(NMots, Mots)) { 
                pid_t pid = fork();
                if (pid == 0) {
                    gererRedirections(); // on fait les redirections avant execution
                    execvp(Mots[0], Mots);
                    perror(Mots[0]);
                    exit(EXIT_FAILURE);
                } else {
                    wait(NULL);
                }
            }
        }
        freeMots();
    } else if (nb_cmds > 1) { // s'il y a des pipes
        int pipes[40]; 
        for (int i = 0; i < nb_cmds - 1; i++) {
            if (pipe(pipes + i*2) < 0) { perror("pipe"); exit(EXIT_FAILURE); }
        }
        
        for (int i = 0; i < nb_cmds; i++) {
            analyseCom(cmd_array[i]);
            if (NMots == 0) { freeMots(); continue; }
            
            pid_t pid = fork();
            if (pid == 0) { // fils
                // relier les pipes
                if (i > 0) dup2(pipes[(i-1)*2], STDIN_FILENO); 
                if (i < nb_cmds - 1) dup2(pipes[i*2 + 1], STDOUT_FILENO); 
                
                // on ferme tous les pipes dans le fils
                for (int j = 0; j < 2*(nb_cmds-1); j++) close(pipes[j]); 
                
                gererRedirections(); 
                
                // si c'est une commande interne dans un pipe, on l'execute puis on sort
                if (execComInt(NMots, Mots)) exit(EXIT_SUCCESS); 
                
                execvp(Mots[0], Mots);
                perror(Mots[0]);
                exit(EXIT_FAILURE);
            }
            freeMots();
        }
        
        // on ferme a nouveau les pipes du pere puis on attend
        for (int i = 0; i < 2*(nb_cmds-1); i++) close(pipes[i]);
        for (int i = 0; i < nb_cmds; i++) wait(NULL);
    }
    
    // liberation propre
    for (int i = 0; i < nb_cmds; i++) free(cmd_array[i]);
    free(save_tmp);
}
