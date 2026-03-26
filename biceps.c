#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <sys/wait.h>

#define HOSTNAME_SIZE 256
#define NBMAXC 10

// var globales pour stocker les mots de la commande
static char **Mots;
static int NMots;

// structure pour les commandes internes
typedef struct {
    char *nom;
    int (*fonc)(int, char **);
} CommandeInterne;

// table des commandes internes
static CommandeInterne TabCom[NBMAXC];
static int NComInt = 0;

// fnct pour copier une chaine, On reserve la memoire avec malloc, copie le contenu de la chaine puis renvoie cette nouvelle chaine
char *copyString(char *s)
{
    char *copy = malloc(strlen(s) + 1);
    if (copy == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strcpy(copy, s);
    return copy;
}

// fnct qui analyse la commande et la decoupe en mots
int analyseCom(char *ligne)
{
    char *tmp = copyString(ligne); // on fait une copie car strsep modifie la chaine
    char *save = tmp; // on garde l'adresse de debut pour free
    char *token;
    int i = 0;
    Mots = malloc(100 * sizeof(char *)); // table de mots (max 100)
    if (Mots == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    token = strsep(&tmp, " \t\n");
    while (token != NULL)
    {
        if (strlen(token) > 0) { // on ignore les mots vides
            Mots[i++] = copyString(token);
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
// commande interne exit
int Sortie(int N, char **P)
{
    (void)N;
    (void)P;
    exit(EXIT_SUCCESS);
}
// ajoute une commande interne dans la table
void ajouteCom(char *nom, int (*fonc)(int, char **))
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
}
// pour afficher les commandes internes
void listeComInt(void)
{
    printf("commandes internes :\n");
    for (int i = 0; i < NComInt; i++) {
        printf("%s\n", TabCom[i].nom);
    }
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
        perror("execvp");
        exit(EXIT_FAILURE);
    } else { // pere
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    char hostname[HOSTNAME_SIZE];
    char *user;
    char *prompt;
    char *ligne;
    size_t taille_prompt;
    char symbole_fin;
    majComInt(); // mise a jour des commandes internes
    listeComInt(); // affichage pour verifier
    user = getenv("USER"); // recup du nom de l'utilisateur via la varibale USER
    if (user == NULL) {
        user = "unknown";
    }
    // recup du nom de la machine
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return EXIT_FAILURE;
    }
    hostname[HOSTNAME_SIZE - 1] = '\0';
    if (getuid() == 0) { // choix du symbole (root # ou pas $)
        symbole_fin = '#';
    } else {
        symbole_fin = '$';
    }
    // construction dynamique du prompt
    taille_prompt = strlen(user) + strlen(hostname) + 4;
    prompt = malloc(taille_prompt * sizeof(char));
    if (prompt == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    snprintf(prompt, taille_prompt, "%s@%s%c ", user, hostname, symbole_fin);
    while (1) {
        ligne = readline(prompt);

        if (ligne == NULL) {
            printf("\nfin\n");
            break;
        }
        analyseCom(ligne); // analyse de la commande
        if (NMots > 0) {
            if (!execComInt(NMots, Mots)) { // si ce n'est pas une commande interne
                execComExt(Mots); // alors on execute une commande externe
            }
        }
        freeMots(); // liberation memoire
        free(ligne);
    }
    free(prompt);
    return EXIT_SUCCESS;
}
