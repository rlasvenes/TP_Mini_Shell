#include "Shell.h"
#include "Evaluation.h"

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


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

int rediriger(expr_t mode, char *fichier, int *fds)
{
  // prépare la redirection en ouvrant le fichier puis en affectant
  // le(s) fds[i] selon le mode - ne réalise pas les redirections


  switch(mode){
    case REDIRECTION_I:
    case REDIRECTION_O:
    case REDIRECTION_A:
    case REDIRECTION_EO:
    case REDIRECTION_E:
    default : fprintf(stderr,"redirection non implémentée\n") ; return 1;
  }
  return 0;
}

int executer_simple(Expression *e, int *fds, int bg)
{
  int status = 1;

  if (e->type != SIMPLE) {
    return -1;
  }

  pid_t pid = fork(); // notre processus
  if (pid == 0) {
    for (size_t i = 0; i < 3; i++) {
      if (fds[i] == i) {
        //rien a faire
      } else {
        //redirection a faire
        dup2(fds[i], i);
      }
    } // fin redirection

    execvp(e->arguments[0], e->arguments);
  } // fin fils

  if (bg == 0) { // pas arriere plan ; 1 == arriere plan
    waitpid(pid, &status, 0);
  }


  // crée un processus pour exécuter la commande

  // le fils réalise les redirections nécessaires (c'est à dire
  // losrque fds[i] != i pour i = 0,1,2) et execute la commande

  // le pere ferme les fichiers nécessaires aux redirections et attend
  // la terminaison du fils si la commande n'est pas en arrière plan

  // retourne 0 si OK - le status encodé autrement

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
    return (evaluer(e->droite, fds, bg) && evaluer(e->gauche, fds, bg));

    case SEQUENCE_OU :
    case SEQUENCE_ET :
    case PIPE :
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
