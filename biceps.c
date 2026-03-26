#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>

#define HOSTNAME_SIZE 256

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
        printf("%s\n", ligne);
        free(ligne);
    }
    free(prompt);
    return EXIT_SUCCESS;
}
