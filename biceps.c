#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>

#define HOSTNAME_SIZE 256

// var globales pour stocker les mots de la commande
static char **Mots;
static int NMots;
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
    char *tmp = copyString(ligne); // copie car strsep modifie la chaine
    char *save = tmp; // on garde l'adresse pour free
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
        if (strlen(token) > 0) {
            Mots[i++] = copyString(token);
        }
        token = strsep(&tmp, " \t\n");
    }
    NMots = i;
    free(save); // liberation de la copie
    return NMots;
}

// fonction pour liberer la memoire des mots
void freeMots()
{
    for (int i = 0; i < NMots; i++) {
        free(Mots[i]);
    }
    free(Mots);
}
int main(void)
{
    char hostname[HOSTNAME_SIZE];
    char *user;
    char *prompt;
    char *ligne;
    size_t taille_prompt;
    char symbole_fin;
    
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
    if (getuid() == 0) { //choix du symbole (root # ou pas $)
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
            printf("commande : %s\n", Mots[0]); // premier mot = commande
            for (int i = 1; i < NMots; i++) {
                printf("argument %d : %s\n", i, Mots[i]);
            }
        }
        freeMots(); // liberation memoire
        free(ligne);
    }
    free(prompt);
    return EXIT_SUCCESS;
}
