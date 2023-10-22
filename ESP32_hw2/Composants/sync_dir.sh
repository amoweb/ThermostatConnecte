#!/bin/bash
# main.sh
# chmod +x main.sh
# cd script permet de copier le fichier "main" du dossier xxx partagé dans le dossier ou il s'exécute

#`echo -n` permet de laisser le curseur sur la même ligne, ce qui permet à l'utilisateur de taper la réponse après la question
#`echo -e` permet de modifier les paramtres du terminal

#option rsync -n pour simuler
cmd=$1
dir=${PWD##*/}
partage="/media/sf_Partage_linux/ESP32_projets"

usage() {
    echo "    Usage: $0 [option]  sans option synchronisation du dossier OSX/'$dir' vers Linux ${PWD}"
 	echo "              -u = synchro dossier Linux ${PWD} vers OSX/'$dir'"    
    exit
}

#if [ -z $1 ]
if [ "$cmd" = "-u" ]
then
	echo -e "      synchronisation du dossier \033[1;31;40m ! Linux => OSX ! \033[0m"
	echo "       Linux ${PWD}"
	echo "    => OSX $partage/'$dir'"
	rsync -rtuvn --exclude 'archives' --exclude 'build' ./ $partage/$dir
	echo -ne "          synchronisation du dossier \033[1;31;40m Linux ${PWD} => OSX/'$dir' ? o/n : \033[0m"
	read choix
	if [ "$choix" = "o" ]  || [ "$choix" = "O" ]
	then
		rsync -rtuv --exclude 'archives' --exclude 'build' ./ $partage/$dir
#		echo "      dossier '$dir' synchronisé de linux vers OSX"
	else
		exit
	fi

elif [ "$cmd" = "-h" ]
then
	usage
	
else
	echo "    aide = h "
	echo "    synchronisation du dossier "
	echo "         OSX $partage/$dir "
	echo -n "       => Linux ${PWD}          ? h/o/n/t/i/l : "
	read choix
	if [ "$choix" = "o" ]  || [ "$choix" = "O" ]
	then
#		echo "      synchronisation du dossier OSX/'$dir' vers Linux ${PWD}"
		rsync -rtuv --exclude '.DS_Store' --exclude 'archives' $partage/$dir ../

	elif [ "$choix" = "i" ]
	then
		echo "      stat synchronisation du dossier OSX/'$dir' vers Linux ${PWD}"
		rsync -rtuvn --stats --exclude '.DS_Store' --exclude 'archives' $partage/$dir ../

	elif [ "$choix" = "l" ]
	then
		echo "      liste tous les fichiers synchronisés du dossier OSX/'$dir' vers Linux ${PWD}"
		rsync -rtuvn --list-only --exclude '.DS_Store' --exclude 'archives' $partage/$dir ../
	
	elif [ "$choix" = "t" ]
	then
		echo "      liste modif synchronisation du dossier OSX/'$dir' vers Linux ${PWD}"
		rsync -rtuvn --exclude '.DS_Store' --exclude 'archives' $partage/$dir ../

	elif [ "$choix" = "n" ]
	then
		usage

	else
		echo "              t = simulation synchronisation OSX/'$dir' vers Linux ${PWD}"
    	echo "              i = stat synchro dossier OSX/'$dir' vers Linux ${PWD}"
		echo "              l = liste tous les fichiers synchro OSX/'$dir' vers Linux ${PWD}"
	    echo "              $0 -u = synchro dossier Linux ${PWD} vers OSX/'$dir'"
	fi

fi

