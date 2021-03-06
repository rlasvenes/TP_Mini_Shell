#include "Shell.h"
#include "Evaluation.h"

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define VRAI (1==1)
#define FAUX (1==0)

#define WRITE 1
#define READ 0

int
verifier(int cond, char *s)
{
    if (!cond)
    {
        perror(s);
        return 0;
    }
    return 1;
}

int my_pipe(Expression *e, int *fds, int bg) {
    verifier(e != NULL, "Null expression");
    //
    // int tube[2];
    // int status;
    //
    // pipe(tube);
    // pid_t pid;
    //
    // pid = fork();
    // if (pid == 0) {
    //     close(tube[1]);
    //     dup2(tube[READ], fds[0]); // gauche écris
    //     close(tube[0]);
    //     execvp(e->arguments[0], e->arguments);
    //     exit(0);
    // }


    // if (pid == 0) { // fils
    //     close(tube[1]);
    //     dup2(tube[READ], fds[0]); // gauche écris
    //     close(tube[0]);
    //     execvp(droite->arguments[0], droite->arguments);
    // } else { // pere
    //     close(tube[0]);
    //     dup2(tube[WRITE], fds[1]); // droite lis
    //     close(tube[1]);
    //     execvp(gauche->arguments[0], gauche->arguments);
    //     // exit(0);
    // }
    //
    // waitpid(pid, &status, WNOHANG);

    return 0;
}

int rediriger(expr_t mode, char *fichier, int *fds)
{
    // prépare la redirection en ouvrant le fichier puis en affectant
    // le(s) fds[i] selon le mode - ne réalise pas les redirections

    int fd;

    // reminder:
    // 0 = stdin = STDIN_FILENO
    // 1 = stdout = STDOUT_FILENO
    // 2 = stderr = STDERR_FILENO

    switch(mode){
        case REDIRECTION_I:
        fd = open(fichier, O_RDONLY);
        fds[STDIN_FILENO] = fd;
        break;
        case REDIRECTION_O:
        fd = open(fichier, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        fds[STDOUT_FILENO] = fd;
        break;
        case REDIRECTION_A:
        fd = open(fichier, O_WRONLY | O_APPEND | O_CREAT, 0644);
        fds[STDOUT_FILENO] = fd;
        break;
        case REDIRECTION_EO:
        fd = open(fichier, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        fds[STDOUT_FILENO] = fds[STDERR_FILENO] = fd;
        break;
        case REDIRECTION_E:
        fd = open(fichier, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        fds[STDERR_FILENO] = fd;
        break;
        default : fprintf(stderr,"redirection non implémentée\n") ; return 1;
    }

    return fd;
}

int executer_simple(Expression *e, int *fds, int bg)
{
    // crée un processus pour exécuter la commande

    // le fils réalise les redirections nécessaires (c'est à dire
    // losrque fds[i] != i pour i = 0,1,2) et execute la commande

    // le pere ferme les fichiers nécessaires aux redirections et attend
    // la terminaison du fils si la commande n'est pas en arrière plan

    // retourne 0 si OK - le status encodé autrement

    int status = 1;

    if (e->type != SIMPLE) {
        return -1;
    }

    printf("e->arguments = %s\n", e->arguments[0]);

    pid_t pid = fork(); // notre processus
    if (pid == 0) {
        for (size_t i = 0; i < 3; i++) {
            if (fds[i] != i) { dup2(fds[i], i); }
        } // fin redirection
        execvp(e->arguments[0], e->arguments);
    } // fin fils

    if (bg == 0) { // pas arriere plan ; 1 == arriere plan
        waitpid(pid, &status, 0);
    }

    return status;
}


int evaluer(Expression *e, int *fds, int bg)
{
    int status;

    if (e == NULL) return 0 ;

    switch(e->type){

        case VIDE :
        exit(0);

        case SIMPLE :
        return executer_simple(e,fds,bg);

        case REDIRECTION_I:
        case REDIRECTION_O:
        case REDIRECTION_A:
        case REDIRECTION_E:
        case REDIRECTION_EO :
        if( !rediriger(e->type, e->arguments[0],fds))
        return 0;
        return evaluer(e->gauche,fds,bg);
        break;

        case BG:
        return evaluer(e->gauche,fds,1); // si on a evalué le flag BG à vrai, alors on le met à 1 (vrai) et on évalue l'arbre gaucher
        // car le symbole '&' ne peut se trouver que à la fin d'une expression, et donc, n'a rien à sa droite

        case SEQUENCE :
        return (evaluer(e->gauche, fds, bg) || executer_simple(e->droite, fds, bg));

        case SEQUENCE_OU : // cmd1 || cmd2 --> execute cmd2 ssi valeur retour de cmd1 != 0
        return (evaluer(e->gauche, fds, bg) && evaluer(e->droite, fds, bg));

        case SEQUENCE_ET : // cmd1 && cmd2 --> execute cmd2 ssi valeur retour de cmd1 == 0
        return (!evaluer(e->gauche, fds, bg) && evaluer(e->droite, fds, bg));

        case PIPE :
        return my_pipe(e, fds, bg); // ?????
        default :
        printf("not yet implemented \n");
        return 1;
    }
}

int
evaluer_expr(Expression *e)
{
    int fds[]={0,1,2}; // initialement pas de redirection
    int bg = 0; // initialement pas en arrière plan
    int status = 0;
    if (e->type == VIDE)
    return 0;
    return evaluer(e,fds,bg);
}
