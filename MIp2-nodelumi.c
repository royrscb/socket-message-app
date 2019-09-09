/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer nodelumi.c que implementa la interfície aplicació-administrador */
/* d'un node de LUMI, sobre la capa d'aplicació de LUMI (fent crides a la */
/* interfície de la capa LUMI -fitxers lumi.c i lumi.h-).                 */
/* Autors: Roy, Quim                                                      */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <stdio.h> o #include "meu.h"     */
/* Incloem "MIp2-lumi.h" per poder fer crides a la interfície de LUMI     */
#include <string.h>
#include <stdio.h>     
#include <fcntl.h>     
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "MIp2-lumi.h"

/* Definició de constants, p.e., #define MAX_LINIA 150                    */

char nomFixAg[32]="MIp2-nodelumi.cfg", nomFixLog[32]="logNode.txt";


void main()
{


	struct agent tauAgents[100]; struct agentEsperant tauAgtsEsp[25];
	char iploc[16] = "0.0.0.0", IP[16], ipTCPser[16], tipus[3], dominiLoc[100]; 
	int portloc = 4096, portUDP, portTCPser, nTauEsp = 0, fdlog = Log_CreaFitx(nomFixLog);

	int nTauAg = LUMI_creaTauAgents(nomFixAg, tauAgents, dominiLoc); if(nTauAg == -1) { printf("ERROR al crear taula agents.\n"); exit(0); }

	int fdsoc = LUMI_CreaSock(iploc, portloc);  if(fdsoc == -1) { printf("ERROR al crear socket UDP\n"); exit(0); }
	LUMI_TrobaAdrSockLoc(fdsoc, iploc, &portloc);
	printf("\nDomini: %s\n@IP:%s PortUDP:%d\n\n", dominiLoc, iploc, portloc );

	int fdTau[2] = {0, fdsoc};

while(1){  char linia[255];



	while(nTauEsp > 0 && LUMI_HaArrivatAlgo(&fdsoc, 1, 10000) == -2){//si triga mes de 10000 microsegons es marca com a desconectat i es notifica al node que estava esperant la resposta

			tauAgents[ LUMI_BuscaAtau(tauAgents, nTauAg, tauAgtsEsp[0].adrMIser) ].conectat=0;
			LUMI_EnviaRespAdr(fdsoc, tauAgtsEsp[0].IPcli , tauAgtsEsp[0].portUDPcli , 2, "0.0.0.0", 0, tauAgtsEsp[0].adrMIcli);

			printf("S'esperava a %s i no contesta\n\n", tauAgtsEsp[0].adrMIser );
			borraDeTaulaEsp(tauAgtsEsp, &nTauEsp, NULL);
	}


	printf("Esperant peticions... (intro per desplegar menu)\n"); 

	while(LUMI_HaArrivatAlgo(fdTau, 2, -1) == 0){

		char opcio[101];

		printf("0: Apagar node\n1: Llistar agents\n2: Mostrar domini node\nEntra nom d'agent per donar d'alta o baixa\n\n"); write(0,">  ",2);
		read(0,opcio,100);

		if( LUMI_HaArrivatAlgo(fdTau, 2, -1) == 0){

			int len = read(0, opcio, 100); 	opcio[len -1]='\0'; //es cambia \n per \0

			if(strlen(opcio)==1 && opcio[0]=='0') { printf("Apagant el node\n\n"); LUMI_tancaSock(fdsoc); exit(0); }
			else if( strlen(opcio)==1 && opcio[0]=='1') { 

				int i; 

				printf("AdrMI     conect     @IP          portUDP\n---------------------------------------------\n");

				for(i=0; i<nTauAg; i++) {

					printf("%s",tauAgents[i].username);
					int j, l=12 - strlen(tauAgents[i].username);
					for(j=0; j<l; j++) printf(" ");
					printf("%d      %s        %d\n", tauAgents[i].conectat, tauAgents[i].IP, tauAgents[i].portUDP );
				}
			}else if( strlen(opcio)==1 && opcio[0]=='2') printf("Domini node: %s\n", dominiLoc);
			else if(strlen(opcio) > 1){

				if(LUMI_BuscaAtau(tauAgents, nTauAg, opcio) == -1){

					strcpy(tauAgents[nTauAg].username, opcio);
					strcpy(tauAgents[nTauAg].IP, "0.0.0.0");
					nTauAg++;

					FILE *fdfix = fopen (nomFixAg, "a+");

					fprintf(fdfix, "%s\n", opcio); 

					fclose(fdfix);

					printf("%s donat d'alta\n", opcio);
 
				}else{

					int i,pos = LUMI_BuscaAtau(tauAgents, nTauAg, opcio);

					for(i=pos; i<nTauAg-1; i++) { strcpy(tauAgents[i].username, tauAgents[i+1].username); tauAgents[i].conectat=tauAgents[i+1].conectat; strcpy(tauAgents[i].IP, tauAgents[i+1].IP); tauAgents[i].portUDP=tauAgents[i+1].portUDP; }
					nTauAg--;

					FILE *fdfix = fopen (nomFixAg, "w");

					fprintf(fdfix, "%s\n", dominiLoc);
					for(i=0; i<nTauAg; i++) fprintf(fdfix, "%s\n", tauAgents[i].username);

					fclose(fdfix);

					printf("%s donat de baixa.\n", opcio);
				}
			}
			printf("\nEsperant peticions... (intro per mes info)\n"); 
		}
	}	


	if (LUMI_RepAlgo(fdsoc, IP, &portUDP, tipus, linia) == -1) printf("Error al rebre peti\n");

	if(!strcmp(tipus,"LC")){ //localitzacio client


		char IPser[16], adrMIcli[101], adrMIser[101]; int portUDPser;
		LUMI_separaAdrMI(linia, adrMIser, adrMIcli); 

		int cli = LUMI_trobaAdrDeTauOrLog(tauAgents, nTauAg, iploc, adrMIser, IPser, &portUDPser);

		char domini[100]; LUMI_separaUsrDomini(adrMIcli, NULL, domini); 
		if(portUDP != 4096 && strcmp(domini, dominiLoc)!=0 ) cli = -3; //si pregunta un agent pero no es del domini

		if(cli == -1) LUMI_EnviaRespAdr(fdsoc, IP, portUDP, 1, "0.0.0.0", 0, adrMIcli);//no existeix
		else if(cli >= 0 && tauAgents[cli].conectat == 0) LUMI_EnviaRespAdr(fdsoc, IP, portUDP, 2,  "0.0.0.0", 0, adrMIcli);//no esta conectat
		else if(cli >=0 || cli == -2){ 

			if(cli >= 0){//si es del domini (ara es preguntara a agent) es posa com a esperant

				//emplena camp taula
				strcpy(tauAgtsEsp[nTauEsp].adrMIcli, adrMIcli);	strcpy(tauAgtsEsp[nTauEsp].adrMIser, adrMIser); 	strcpy(tauAgtsEsp[nTauEsp].IPcli, IP); 	tauAgtsEsp[nTauEsp].portUDPcli = portUDP; 

				nTauEsp++;
			}

			LUMI_EnviaAlgo(fdsoc, IPser, portUDPser, "LC", linia); 

		}else if(cli == -3) printf("Peticio de localitzacio de %s ignorada per no ser del domini\n", adrMIcli);
	}else if(!strcmp(tipus, "RL")){//resposta localitzacio
		
		char IPcli[16], adrMIcli[101]; int portUDPcli;

		int inf = LUMI_tradueixAdr(fdsoc, linia, ipTCPser, &portTCPser, adrMIcli); //troba ladreça TCP i MI de qui la solicita

		int cli = LUMI_trobaAdrDeTauOrLog(tauAgents, nTauAg, iploc, adrMIcli, IPcli, &portUDPcli); //troba adreça UDP de qui ha solicitat ladreça TCP a log o taula

		if(cli == -1) printf("Error al resoldre adrMI:%s\n", adrMIcli);
		else if(cli >= 0 || cli == -2) {

			if(nTauEsp > 0) borraDeTaulaEsp(tauAgtsEsp, &nTauEsp, adrMIcli, 0);

			if (LUMI_EnviaRespAdr(fdsoc, IPcli, portUDPcli, inf, ipTCPser, portTCPser, adrMIcli) == -1) printf("Error al enviar resposta\n");

		}else if(cli == -3)	printf("RL cap a %s no esperada.\n", adrMIcli);
	}else if(!strcmp(tipus,"RC")){ //Registre client
		
		char username[100];

		LUMI_separaUsrDomini(linia, username, NULL);

		if( LUMI_actuTauAgents(tauAgents, nTauAg, username, IP, portUDP) == -1) LUMI_EnviaAlgo(fdsoc, IP, portUDP, "RR", "0");
 		else{

			printf("Client %s registrat.\n", username );
			LUMI_EnviaAlgo(fdsoc, IP, portUDP, "RR", "1"); 

		}
	}else if(!strcmp(tipus, "DC")){//Desregistre client

		char username[100];

		LUMI_separaUsrDomini(linia, username, NULL);

		if( LUMI_actuTauAgents(tauAgents, nTauAg, username, NULL, NULL) == -1) LUMI_EnviaAlgo(fdsoc, IP, portUDP, "RD", "0");
		else{

			printf("Client %s desregistrat.\n", username);
			LUMI_EnviaAlgo(fdsoc, IP, portUDP, "RD", "1"); 

		}	
	}else printf("ERROR '%s%s' rebut.\n", tipus,linia);



	printf("\n");
}


	LUMI_tancaSock(fdsoc);
	Log_TancaFitx(fdlog);

}



// borra de la taula de espera un agent esperant i reordena la taula perqueno hi hagin espais buits, si adrMI es NULL esborra el primer.
int borraDeTaulaEsp( struct agentEsperant *tau,int *n, char *adrMI){

	int pos=0;

	if(adrMI != NULL) while(strcmp(tau[pos].adrMIcli, adrMI) != 0) pos++;

	int i; 
	for(i=pos; i<((*n)-1); i++){

		strcpy(tau[i].adrMIcli, tau[i+1].adrMIcli);
		strcpy(tau[i].adrMIser, tau[i+1].adrMIser); 
		strcpy(tau[i].IPcli, tau[i+1].IPcli); 
		tau[i].portUDPcli = tau[i+1].portUDPcli; 

	}

	*n = *n -1;

	return pos;
}