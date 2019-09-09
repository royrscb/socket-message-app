/**************************************************************************/
/*                                                                        */
/* P1 - MI amb sockets TCP/IP - Part II                                    */
/* Fitxer p2p.c que implementa la interfície aplicació-usuari de          */
/* l'aplicació de MI, sobre la capa d'aplicació de MI (fent crides a la   */
/* interfície de la capa MI -fitxers mi.c i mi.h-).                       */
/* Autors: Roy, Quim                                                           */
/*                                                                        */
/**************************************************************************/


/* Inclusió de llibreries, p.e. #include <stdio.h> o #include "meu.h"     */
#include <string.h>
#include <stdio.h>     
#include <stdlib.h>   
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "MIp2-mi.h"
#include "MIp2-lumi.h"

char IPnode[16], adrMIloc[101]="Roy_Ros_Cobo";
int portUDPnode=4096;


void crearUDPiRegistrarseAlNode(int *fdsocUDP, char *iploc,  int *portUDPloc, char *adrMIloc);
int demanarIrebreAdrTCPaNode(int fdsocUDP, char* adrMIrem, char* iprem, int *portrem);


void main(){	MI_getIPloc(IPnode);

char nickloc[300]="Roy\n", nickrem[300], iploc[16]="0.0.0.0", iprem[16];
char adrMIrem[101], tipus[3], linia[204];
int fdsocTCP, fdsocUDP, portrem=0, portUDPloc=0, fdlog = Log_CreaFitx("logAgent.txt");


write(0,"Quin nickname vols tenir? ",26); int len = read(0, nickloc, 300); nickloc[len-1]='\0'; 


char sino[2]="y"; while(sino[0]=='y'){//whileeeeeeee

int portTCPloc=0;

crearUDPiRegistrarseAlNode(&fdsocUDP, iploc, &portUDPloc, adrMIloc);

begin:

write(0, "\nEntra @MI de qui et vulguis conectar o 'fi' per acabar: ",57); 

int on = MI_HaArribatPetiConv(fdsocUDP);
if(on == 0){	//teclaaat

	int lenAux = read(0, adrMIrem, 100);  adrMIrem[lenAux-1]='\0'; //es canvia '\n' per '\0'
	if(adrMIrem[0]=='f' && adrMIrem[1]=='i') goto exit;

	int inf = demanarIrebreAdrTCPaNode(fdsocUDP, adrMIrem, iprem, &portrem);

	if(inf==3) printf("Usuari ocupat\n");else if(inf==2) printf("Usuari offline\n");else if(inf==1) printf("Usuari no existeix\n");
	else if(inf==0)	if( (fdsocTCP = MI_DemanaConv(iprem, portrem, iploc, &portTCPloc, nickloc, nickrem)) == -1) { printf("ERROR: demana conversa\n");  goto exit; }

	if(inf!=0) goto begin;

}else if(on == fdsocUDP){ //socket UDP

	if( (fdsocTCP = MI_IniciaEscPetiRemConv(portTCPloc)) == -1) { printf("ERROR: crear socket TCP\n"); exit(0); }
	MI_getPortLoc(fdsocTCP, &portTCPloc);

	LUMI_RepAlgo(fdsocUDP, IPnode, &portUDPnode, tipus, linia); LUMI_separaAdrMI(linia, adrMIloc, adrMIrem);
	LUMI_EnviaRespAdr(fdsocUDP, IPnode, portUDPnode, 0, iploc, portTCPloc , adrMIrem);

	//if( MI_AcabaEscPetiRemConv(fdsocTCP) == -1){ printf("ERROR: close socket servidor\n");  exit(0); }
	if( (fdsocTCP = MI_AcceptaConv(fdsocTCP, iprem, &portrem, iploc, &portTCPloc, nickloc, nickrem)) == -1) { printf("ERROR: accepta conversa\n");  exit(0); }
}

printf("Port local:%d        @IP local:%s\n", portTCPloc, iploc);
printf("Port remot:%d        @IP remot:%s\n\n", portrem, iprem);



//--------------------------------------------------------PART ESCRITURA-----------------------------------------------------------



printf("exit per acabar conexio\n\n");

printf("Chat amb %s\n", nickrem);
printf("---------------------------------------------\n");

while(1){

	int lentxt;
	char msg[300]; 

	int listSock[3]={0, fdsocTCP, fdsocUDP};
	on = LUMI_HaArrivatAlgo(listSock, 3, -1);

	if(on == -1) break;
	else if(on == 0){ //si es teclat--------------

		lentxt = read(0,msg,300); msg[lentxt]='\0';
		
		if(!strncmp(msg,"exit",4)) break;

		MI_EnviaLinia(fdsocTCP, msg);

	}else if(on == fdsocTCP){ //si es socket -------------

		if( MI_RepLinia(fdsocTCP, msg) == -2) break;
		else printf("%s:%s\n", nickrem, msg);
	
	}else if(on == fdsocUDP){

		LUMI_RepAlgo(fdsocUDP, IPnode, &portUDPnode, tipus, linia);	LUMI_separaAdrMI(linia, adrMIloc, adrMIrem);
		LUMI_EnviaRespAdr(fdsocUDP, IPnode, portUDPnode, 3, "0.0.0.0", 0, adrMIrem );
	}
}

exit:

MI_AcabaConv(fdsocTCP);
LUMI_EnviaPetiRegOrDesreg(fdsocUDP, IPnode, portUDPnode, adrMIloc, 0);
LUMI_tancaSock(fdsocUDP);

printf("Vols tornar a conectarte? (y/n) "); scanf("%s", sino);
printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

printf("\nFi de conexio\n\n");

}





void crearUDPiRegistrarseAlNode(int *fdsocUDP, char *iploc,  int *portUDPloc, char *adrMIloc){

	*fdsocUDP = LUMI_CreaSock(iploc, *portUDPloc);
	if( LUMI_TrobaAdrSockLoc(*fdsocUDP, iploc, portUDPloc) == -1) printf("Error al trobar adr sock UDP local\n");	
	printf("IP local: %s 	Port UDP local: %d\n\n",iploc, *portUDPloc);

	write(0,"Quina adrMI tens? ",18); int len = read(0, adrMIloc, 100); adrMIloc[len-1]='\0';

	char domini[101];
	LUMI_separaUsrDomini(adrMIloc, NULL, domini);
	if(ResolDNSaIP(domini, IPnode) == -1) { printf("Error al resoldre DNS\n"); exit(0); } 

	int i=0;
	while(i<3){

		LUMI_EnviaPetiRegOrDesreg(*fdsocUDP, IPnode, portUDPnode, adrMIloc, 1);//es registra al servidor
		if( LUMI_HaArrivatAlgo(fdsocUDP, 1, 100000) != -2 ) break;

		i++;
	}

	char tipus[3], linia[103];
	if(i==3) { printf("ERROR al registrar-se al servidor\n"); exit(0); }
	else LUMI_RepAlgo(*fdsocUDP, IPnode, &portUDPnode, tipus, linia );
	if(tipus[0]=='R' && tipus[1]=='R' && linia[0]=='0') { printf("ERROR no estas registrat al servidor!\n"); exit(0); }
}

int demanarIrebreAdrTCPaNode(int fdsocUDP, char* adrMIrem, char* iprem, int *portrem){

	char tipus[3]="LC", linia[204]; 
	int inf, i=0, ll[1]={fdsocUDP};

	if(!strcmp(adrMIrem, adrMIloc)) { printf("ERROR no et pots conectar amb tu mateix!\n"); return -1; }
	strcpy(linia, adrMIrem); linia[strlen(linia)]=':'; strcpy(linia+strlen(adrMIrem)+1, adrMIloc); 

	while(i<3){

		LUMI_EnviaAlgo(fdsocUDP, IPnode , portUDPnode, tipus, linia);
		if( LUMI_HaArrivatAlgo(ll, 1, 1000000) != -2 ) break;

		i++;
	}if(i==3) printf("ERROR: no s'ha rebut cap resposta del node.\n");
	else if( LUMI_RepAlgo(fdsocUDP, IPnode, &portUDPnode, tipus, linia) == -1) printf("Error al rebre resposta del node\n"); 



	return LUMI_tradueixAdr(fdsocUDP,linia, iprem, portrem, adrMIrem); printf("xxxxxxxx:%s\n", adrMIloc);
}