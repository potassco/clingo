% Programme de fouille de motifs séquentiels (occurrences minimales)
% author : T. Guyet
% date : 02/2014
% 
% parametres de la ligne de commande:
%   * -c th=? : fixe le seuil (3 par default)
%   * -c ml=? : profondeur de la recherche (taille max des motifs) (4 par default)
%
% ces deux valeurs sont remplacées par des constantes du programme :
%   - th : seuil de frequence, valeur par defaut 3 : elle peut être modifiée par eds parametres lors de l'appel du programme
%   - nbs : nombre de symboles : defini lors de la construction de la séquence
#const th = 3.

% liste des symboles fréquents
symb(S) :- th { seq(P,S) }, S=1..nbs.

%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% partie visualisation
%%%%%%%%%%%%%%%%%%%%%%%%%%%

%#show occ/3. %ne pas afficher les occ pour l'option project 
#show pattern/2.
%#show symb/1.

%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% partie Contrôle de la résolution
%%%%%%%%%%%%%%%%%%%%%%%%%%%

#script(python)
import sys, os
from clingo import *
if sys.version_info[0] >= 3:
    import io as StringIO
else:
    import StringIO

modelid = 0

patterns=list()

def readsequence(filename):
    seq = list()
    src = StringIO.StringIO("""1 5
2 2
3 5
4 2
5 5
6 2
7 1
8 2
9 4
10 2
11 1
12 6
13 1
14 2
15 5
16 3
17 3
18 1
19 3
20 2
""")
    if src is None:
        print ("invalid filename:" + str(filename))
        return None
    predpos=0
    for line in src:
        lineelem = line.rstrip('\n\r').lstrip(' ').split(' ')
        pos=lineelem[0]
        if( pos==predpos ) :
            print ("invalid sequence : simultaneous events are not allowed")
            return None
        predpos=pos
        seq.append( int(lineelem[1]) )
    src.close()
    return seq

def genASPsequence(seq, prg):
    i=1
    #ajout de la constante pour le nombre de symboles
    s="#const nbs = "+str(max(seq)) +"."
    prg.add("base",[], s )
    for n in seq:
        s='seq('+str(i)+","+str(n)+').'
        prg.add("base", [], s)
        i=i+1

def on_model(model):
    pattern={} # map qui associe le numero d'instance à un numero de structure
    
    for atom in model.symbols(atoms=True):
        if atom.name()=="pattern":
            args = atom.arguments
            pattern[ args[0] ]=args[1]
    
    found=False
    for i in patterns:
        if i==pattern:
            found=True
            break
    if not found:
        patterns.append( pattern )

def main(prg):
    #Chargement du jeu de données
    theseq = readsequence("we_cmp.seq.minepi")
    genASPsequence(theseq, prg)
    prg.configuration.solve.project = 1
    prg.configuration.solve.models  = 0
    d = prg.get_const("ml")
    if d is None:
        d=4
    else:
        d = d.number
    
    prg.ground([("base",[])])
    prg.solve_refactored()
    
    for k in range(2,d+1):
        prg.ground([("incr",[k])])
        prg.solve_refactored()
#end.

%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% partie Base
%%%%%%%%%%%%%%%%%%%%%%%%%%%

% génération de tous les motifs de taille 1
1{ pattern(1, P) : symb(P) } 1.

% on genere les occurrences pour chacun des patterns-singletons : le numero de l'instance est la position initiale (unique) !
{ occ(P, 1, P) : seq(P, S) } :- pattern(1,S).

% pour imposer d'avoir nécessairement toutes les occurrenres d'un motifs (et pas simplement des sous-ensembles):
:- seq(P, S), not occ(P, 1, P), pattern(1,S).

%%%%%%%%%%%%
%% cas des tailles 2
%1{ pattern(2, P) : symb(P) } 1.

% extention des occurrences du motif
%0{ occ(I,2,Q) : seq(Q,S), pattern(2,S), Q>P } 1 :- occ(I, 1, P).

%:- occ(I, 1, P), occ(I, 2, Q), seq(P',S), pattern(2,S), P<P', P'<Q.
%:- occ(I, 2, P), occ(J, 2, P'), P=P', I<J. %deux occurrences ne peuvent pas avoir la même fin
%:- occ(I, 1, P), occ(J, 1, P'), occ(I, 2, Q), occ(J, 2, Q'), P<P', Q'<Q, I!=J. %pas d'inclusion des occurrences d'un meme motif

% contrainte de frequence
%idocc(I,2) :- occ(I,2,Q).
%:- { idocc(I,2) } th-1.

%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% partie incrementale
%%%%%%%%%%%%%%%%%%%%%%%%%%%


#program incr(n).
%#external pattern(1..n, 1..nbs).
%#external occ(I, n-1, 1..nbs)

% extension du motif
1{ pattern(n, P) : symb(P) } 1.

% extension des occurrences du motif
% ATTENTION : il faut mettre une contrainte de type 0 { ... } 1
%   1: il faut en generer au plus 1 dans un AS
%   0: il faut laisser la possibilite de ne pas pouvoir completer l'occurrence !
0{ occ(I,n,Q) : seq(Q,S), pattern(n,S), Q>P } 1 :- occ(I, n-1, P).
%0{ possocc(I,n,Q) : seq(Q,S), pattern(n,S), Q>P } 1 :- occ(I, n-1, P).

%#show occ/3.

%
%#external closed(1..nbs).
%% pour savoir si le motif plus petit est fermé par l'ajout de S:
%nbnext(I,n-1,N) :- occ(I, n-1, P), N={ occ(I,n,Q) }.
%closed(S) :- { nbnext(I,n-1,N) : N=0 } 0, pattern(n,S).
%#show closed/1.


% On prend le premier symbole de la fin du motif juste apres la fin d'une occurrence du motif "inferieur" :
:- occ(I, n-1, P), occ(I, n, Q), seq(P',S), pattern(n,S), P<P', P'<Q.
%:- occ(I, n-1, P), occ(I, n, Q), occ(J, 1, P'), J!=I, P<P', P'<Q.

:- occ(I, n, P), occ(J, n, P'), P=P', I<J. %deux occurrences ne peuvent pas avoir la même fin
:- occ(I, 1, P), occ(J, 1, P'), occ(I, n, Q), occ(J, n, Q'), P<P', Q'<Q, I!=J. %pas d'inclusion des occurrences d'un meme motif

%test(n) :- occ(I, n-1, P), occ(I, n, Q), seq(P',S), pattern(n,S), P<P', P'<Q.
%#show test/1.

% contrainte de frequence
idocc(I,n) :- occ(I,n,Q).
:- { idocc(I,n) } th-1.



