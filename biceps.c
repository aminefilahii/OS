#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include "gescom.h"

#define HOSTNAME_SIZE 256

// fnct pour gerer l'interruption control-c sans quitter
void sigint_handler(int sig) {
    (void)sig; // ignore le signal
    printf("\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay(); // on reaffiche le prompt
}

int main(void)
{
    char hostname[HOSTNAME_SIZE];
    char *user;
    char *prompt;
    char *ligne;
    size_t taille_prompt;
    char symbole_fin;
    
    signal(SIGINT, sigint_handler); // modification pour control-c

    majComInt(); // mise a jour des commandes internes
    
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
            printf("\nfin\n"); // sortie propre sur control-d
            break;
        }
        
        if (strlen(ligne) > 0) {
            add_history(ligne); // on ajoute a l'historique
            
            char *tmp_ligne = ligne;
            char *sous_commande;
            
            // on decoupe avec le delimiteur ; pour le sequentiel
            while ((sous_commande = strsep(&tmp_ligne, ";")) != NULL) {
                if (strlen(sous_commande) > 0) {
                    traiterPipeline(sous_commande); // execute la logique des pipes et redirections
                }
            }
        }
        free(ligne);
    }
    free(prompt);
    return EXIT_SUCCESS;
}
