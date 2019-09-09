/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer lumi.c que implementa la capa d'aplicació de LUMI, sobre la     */
/* de transport UDP (fent crides a la interfície de la capa UDP           */
/* -sockets-).                                                            */
/* Autors: Roy, Quim                                                      */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <sys/types.h> o #include "meu.h" */
/*  (si les funcions externes es cridessin entre elles, faria falta fer   */
/*   un #include "MIp2-lumi.h")                                           */
#include <string.h>
#include <time.h>
#include <stdio.h>       
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <fcntl.h>      
#include <sys/stat.h>
#include "MIp2-lumi.h"

/* Definició de constants, p.e., #define MAX_LINIA 150                    */

int fdlog;

/* Declaració de funcions internes que es fan servir en aquest fitxer     */
/* (les seves definicions es troben més avall) per així fer-les conegudes */
/* des d'aqui fins al final de fitxer.                                    */
/* Com a mínim heu de fer les següents funcions internes:                 */

int UDP_CreaSock(const char *IPloc, int portUDPloc);
int UDP_EnviaA(int Sck, const char *IPrem, int portUDPrem, const char *SeqBytes, int LongSeqBytes);
int UDP_RepDe(int Sck, char *IPrem, int *portUDPrem, char *SeqBytes, int LongSeqBytes);
int UDP_TancaSock(int Sck);
int UDP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portUDPloc);
int HaArribatAlgunaCosaEnTemps(const int *LlistaSck, int LongLlistaSck, int Temps);
int ResolDNSaIP(const char *NomDNS, char *IP);
int Log_CreaFitx(const char *NomFitxLog);
int Log_Escriu(int FitxLog, const char *MissLog);
int Log_TancaFitx(int FitxLog);

/* Definicio de funcions EXTERNES, és a dir, d'aquelles que en altres     */
/* fitxers externs es faran servir.                                       */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa LUMI.                                  */
/* Les funcions externes les heu de dissenyar vosaltres...                */


/* Crea un socket UDP a l’@IP “IPloc” i #port UDP “portUDPloc”            */
/* (si “IPloc” és “0.0.0.0” i/o “portUDPloc” és 0 es fa/farà una          */
/* assignació implícita de l’@IP i/o del #port UDP, respectivament).      */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.   															  */
int LUMI_CreaSock(const char *IPloc, int portUDPloc){

	return UDP_CreaSock(IPloc, portUDPloc);
}

/* Donat el socket UDP d’identificador “Sck”, troba l’adreça d’aquest     */
/* socket, omplint “IPloc*” i “portUDPloc*” amb respectivament, la seva   */
/* @IP i #port TCP.                                                       */
/* "IPloc*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int LUMI_TrobaAdrSockLoc(int Sck, char *IPloc, int *portUDPloc){

	return UDP_TrobaAdrSockLoc(Sck, IPloc, portUDPloc);
}

/*Tanca el socket Sck i retorna -1 en cas d'error i 1 en cas d'exit.*/
int LUMI_tancaSock(int Sck){

	return UDP_TancaSock(Sck);
}


/*Escolta durant el temps temps fins si arriba alguna cosa per algun 
socket de la llista ll.
retorna -1 en cas d'error, -2 en cas de esgotarse el temps i 
el identificador del socket pel qual s'ha rebut alguna cosa en cas de
que es rebi alguna cosa.												  */
int LUMI_HaArrivatAlgo(const int* ll, int lenll, int temps){

	return HaArribatAlgunaCosaEnTemps(ll, lenll,  temps);
}


/*rep alguna cosa per el socket Sck.
Omple els parametres IPcli i portUDPcli.
Tambe omple els parametres tipus amb el tipus de cosa rebuda
i linia amb el que s'ha rebut.
tots aquests apuntadors a char son strings de c acabats amb \0.
Retorna -1 en cas d'error i la longitud enviada en cas d'exit.					*/
int LUMI_RepAlgo(int Sck, char *IPcli, int *portUDPcli, char *tipus, char *linia){

	char liniaAUX[2+100+1+100+1]; int len;

	if( (len = UDP_RepDe(Sck, IPcli, portUDPcli, liniaAUX, 2+100+1+100)) == -1) return -1;

	strncpy(tipus, liniaAUX, 2); tipus[2]='\0';

	int i;	
	for(i=0; i<len-2; i++) linia[i] = liniaAUX[i+2];	linia[len-2]='\0';

	return len;
}

/*Envia un string de c que consta de tipus(2bytes)+linia per el socket Sck
a la direccio IP, portUDP.
Linia es un string de c de com a maxim 100 sense incloudre fi de linia
acabat amb '\0' i mai amb '\n'.
retorna -1 en cas d'error i la longitud enviada en cas d'exit					*/
int LUMI_EnviaAlgo(int Sck, char *IP, int portUDP, char* tipus, char *linia){

	char liniaAUX[2+100+1+100+1];
	strcpy(liniaAUX, tipus); 
	strcpy(liniaAUX+2, linia); 

	return UDP_EnviaA(Sck, IP, portUDP, liniaAUX, strlen(liniaAUX));
}

/*Descodifica la linia que s'ha rebut amb la resposta de la adreça TCP.
Omple IPser i portTCPser amb la @ip del servidor i el
#port del servidor TCPs l'@ip es un string de c acabat amb '\0'c
Retorna inf que es un int que identifica l'estat del servidor.					*/
int LUMI_tradueixAdr(int Sck, char *linia, char *IPser, int *portTCPser, char *adrcli){

	int inf=linia[0]-'0'; 

	int lenIP=0;
	while(linia[1+lenIP]!=':') lenIP++;

	strncpy(IPser, linia+1, lenIP); IPser[lenIP]='\0';

	int lenPort = 0;
	while(linia[1+lenIP+1+lenPort]!=':') lenPort++;

	linia[1+lenIP+1+lenPort]='\0'; *portTCPser = atoi(linia+1+lenIP+1);  //estic cambiant el : per \0

	strcpy(adrcli, linia+1+lenIP+1+lenPort+1);

	return inf;
}

/*Envia resposta de una adreça TCP que se li ha solicitat previament
(cli es refereix a el client que l'ha solicitat i pot ser un node o un agent)
Tots els char* son strings de c acabats sempre amb '\0'.
Retorna -1 en cas d'error i 1 en cas d'exit											*/
int LUMI_EnviaRespAdr(int Sck, char *IPcli, int portUDPcli, int inf, char *IPser, int portTCPser, char *adrMIcli ){

	char adrTCPser[2+1+15+1+5+1+100+1]="RL"; adrTCPser[2]=inf+'0';

	strcpy(adrTCPser+2+1, IPser); adrTCPser[strlen(adrTCPser)]=':';
	sprintf(adrTCPser+strlen(adrTCPser), "%d", portTCPser); adrTCPser[strlen(adrTCPser)]=':';
	strcpy(adrTCPser+strlen(adrTCPser), adrMIcli);

	if (UDP_EnviaA(Sck, IPcli, portUDPcli, adrTCPser, strlen(adrTCPser)) == -1) return -1;

	return 1;
}


/* Envia una peticio de registre o desregistre al node ( registre si reg_desreg=1 i desregistre si es 0
a traves del socket Sck.
Tots els strings son strings de c acabats amb el caracter \0
reorna -1 en cas d'error i 1 en cas d'exit.												 */
int LUMI_EnviaPetiRegOrDesreg(int Sck, char * ipNode, int portUDPnode, char *adrMI, int reg_desreg){


	char liniaAUX[2+100+1]="*C";

	if(reg_desreg == 1) liniaAUX[0]='R'; 
	else if(reg_desreg == 0) liniaAUX[0]='D';

	strcpy(liniaAUX+2, adrMI); 

	if( UDP_EnviaA(Sck, ipNode, portUDPnode, liniaAUX, 2+strlen(adrMI)) == -1) return -1;

	return 1;
}





/* Omple la taula tau amb els noms dels agents que hi ha donats d'alta al servidor
Omple la taula tau i n amb la mida de la taula actualitzada.
Retorna -1 en cas d'error i la mida de la taula en cas d'exit.						*/
int LUMI_creaTauAgents(char * nomFix, struct agent *tau, char *domini){

	FILE *fdFix = fopen(nomFix, "r"); char adrAUX[101]; if(fdFix == NULL) return -1;

	fgets(domini,100, fdFix); domini[strlen(domini)-1] = '\0';

	int i=0; 
	while( fgets(adrAUX,100, fdFix)!=NULL ){

		adrAUX[strlen(adrAUX)-1] = '\0';

		strcpy( tau[i].username, adrAUX );
		strcpy(tau[i].IP, "0.0.0.0");

		i++;
	}

	if( fclose(fdFix) == -1) return -1;

	return i;
}


/* Actualitza la taula tau registrant un client com a conectat i inserint la seva IP
i el portUDP o desregistrant un client	com a desconectat si IP i portUDP son NULL.

retorna -1 en cas d'error i 1 en cas d'exit													 */
int LUMI_actuTauAgents(struct agent *tau, const int n, char *username, char *IP, int portUDP){


	int i = LUMI_BuscaAtau(tau, n, username);

	if(i==-1) return -1;

	if(IP == NULL) tau[i].conectat = 0;
	else{

		tau[i].conectat = 1;
		strcpy(tau[i].IP, IP);
		tau[i].portUDP = portUDP;

	}

	return 1;
}


/* Busca un agent a la taula tau per adreça MI.
retorna -1 en cas d'error i la posicio a la taula en cas d'exit.
 */
int LUMI_BuscaAtau(struct agent *tau, const int n, char *username){

	int i=0;

	while( i<n && strcmp(tau[i].username, username) ) i++;

	if( i == n )	return -1;
	else 			return i;
}



/* troba adreça UDP de quin ha de ser el seguent punt de la informacio a partir de una adreça MI
si no es del domini retorna l'adreça del seguent node y si ho es la busca a la taula.
Omple IP i portUDP amb l'adreça.
Retorna la posicio de la taula local en cas de que sigui del domini i sigui a la taula.
Retorna -1 en cas d'error.
Retorna -2 en cas de que no sigui del domini.				*/
int LUMI_trobaAdrDeTauOrLog(struct agent *tau, const int n, char *IPLOC, char *adrMI, char *IP, int *portUDP){

	char username[100], domini[100];
	if(LUMI_separaUsrDomini(adrMI, username, domini) == -1) return -1;

	if( ResolDNSaIP(domini, IP) == -1) return -1; 

	if(strcmp(IP, IPLOC) != 0) {	//si no es de aquest domini senvia la peticio al node del domini

		*portUDP = 4096;

		return -2;

	}else{	//sii sii es de aquest domini miro a la taula

		int cli = LUMI_BuscaAtau(tau, n, username);

		if(cli == -1) return -1;
		else{ 

			strcpy(IP, tau[cli].IP);		
			*portUDP = tau[cli].portUDP; 

			return cli;
		}
	}	
}

/* Separa linia en adreça preguntada i adreça del preguntador */
void LUMI_separaAdrMI(char *linia, char *adrPreguntada, char *adrPreguntador){

	int i=0;

	while(linia[i]!=':') i++;

	if(adrPreguntada != NULL)	{ strncpy(adrPreguntada, linia, i); adrPreguntada[i]='\0'; }
	else 	while(linia[i]!=':') i++;

	strcpy(adrPreguntador, linia+i+1);
}

/* Separa la adrMI en el username i el domini que te tot omplint els camps.
Retorna -1 en cas d'error i 1 en cas d¡exit */
int LUMI_separaUsrDomini(char *adrMI, char *username, char *domini){

	int i=0, len = strlen(adrMI);
	while(i<len && adrMI[i]!='@') i++;
	if(i==len) return -1; 

	if(username != NULL) 	{strncpy(username, adrMI, i); username[i]='\0';}
	if(domini != NULL)		strcpy(domini, adrMI+1+i);

	return 1;
}

//-----------------------------------------------------------------------------------------------------------------------------
/* Definicio de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer.                                  */


/* Crea un socket UDP a l’@IP “IPloc” i #port UDP “portUDPloc”            */
/* (si “IPloc” és “0.0.0.0” i/o “portUDPloc” és 0 es fa/farà una          */
/* assignació implícita de l’@IP i/o del #port UDP, respectivament).      */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int UDP_CreaSock(const char *IPloc, int portUDPloc){
	
	struct sockaddr_in adr; 	int fdsoc;

	adr.sin_family=AF_INET;
	adr.sin_port=htons(portUDPloc);
	adr.sin_addr.s_addr=inet_addr(IPloc);
	bzero(&(adr.sin_zero),8);

	if( (fdsoc=socket(PF_INET, SOCK_DGRAM, 0)) == -1) return -1; 
	if(bind(fdsoc, (struct sockaddr*) &adr, sizeof(adr)) == -1) return -1;
	
	return fdsoc;
}

/* Envia a través del socket UDP d’identificador “Sck” la seqüència de    */
/* bytes escrita a “SeqBytes” (de longitud “LongSeqBytes” bytes) cap al   */
/* socket remot que té @IP “IPrem” i #port UDP “portUDPrem”.              */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud >= LongSeqBytes bytes       */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int UDP_EnviaA(int Sck, const char *IPrem, int portUDPrem, const char *SeqBytes, int LongSeqBytes)
{
	struct sockaddr_in adr; 

	adr.sin_family=AF_INET;
	adr.sin_port=htons(portUDPrem);
	adr.sin_addr.s_addr=inet_addr(IPrem);
	bzero(&(adr.sin_zero),8);

	printf(">>> ENVIA %d: (", LongSeqBytes); int i; for(i=0;i<LongSeqBytes;i++)printf("%c", SeqBytes[i]); printf(") a port: %d\n", portUDPrem);

	char log[500];
	sprintf(log, ">>> ENVIA %d: (%s) a port: %d e IP: %s\n", LongSeqBytes, SeqBytes, portUDPrem, IPrem);
	Log_Escriu(fdlog, log);

	return sendto(Sck, SeqBytes, LongSeqBytes,0, (struct sockaddr*) &adr, sizeof(adr));
}

/* Rep a través del socket UDP d’identificador “Sck” una seqüència de     */
/* bytes que prové d'un socket remot i l’escriu a “SeqBytes*” (que té     */
/* una longitud de “LongSeqBytes” bytes).                                 */
/* Omple "IPrem*" i "portUDPrem*" amb respectivament, l'@IP i el #port    */
/* UDP del socket remot.                                                  */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* "SeqBytes*" és un vector de chars qualsevol (recordeu que en C, un     */
/* char és un enter de 8 bits) d'una longitud <= LongSeqBytes bytes       */
/* Retorna -1 si hi ha error; el nombre de bytes rebuts si tot va bé.     */
int UDP_RepDe(int Sck, char *IPrem, int *portUDPrem, char *SeqBytes, int LongSeqBytes)
{
	struct sockaddr_in adr; int len = sizeof(adr);

	int ret = recvfrom(Sck, SeqBytes, LongSeqBytes, 0, (struct sockaddr*) &adr, &len);

	strcpy(IPrem, inet_ntoa(adr.sin_addr));
	*portUDPrem=ntohs(adr.sin_port);

	printf("<<< REP %d: (", ret); int i; for(i=0;i<ret;i++)printf("%c", SeqBytes[i]); printf(") de port: %d\n", *portUDPrem);

	SeqBytes[ret]='\0';
	char log[500];
	sprintf(log, "<<< REP %d: (%s) de port: %d e IP: %s\n", ret, SeqBytes, *portUDPrem, IPrem);
	Log_Escriu(fdlog, log);

	return ret;
}

/* S’allibera (s’esborra) el socket UDP d’identificador “Sck”.            */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_TancaSock(int Sck)
{ 
	return close(Sck)*2+1; 
}

/* Donat el socket UDP d’identificador “Sck”, troba l’adreça d’aquest     */
/* socket, omplint “IPloc*” i “portUDPloc*” amb respectivament, la seva   */
/* @IP i #port UDP.                                                       */
/* "IPloc*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portUDPloc)
{
	struct sockaddr_in adr;
	int len = sizeof(adr);

	if( getsockname(Sck, (struct sockaddr*) &adr,&len) == -1) return -1;
	

	struct ifaddrs *ifa; struct sockaddr_in *sa; getifaddrs (&ifa);
	for (; ifa; ifa = ifa->ifa_next) 
	    if (ifa->ifa_addr->sa_family==AF_INET) {
	        sa = (struct sockaddr_in *) ifa->ifa_addr;

	        if( strncmp(inet_ntoa(sa->sin_addr), "127.0.0.1", 9) != 0 )	strcpy(IPloc,inet_ntoa(sa->sin_addr));	        
	    }
	freeifaddrs(ifa);
	*portUDPloc=ntohs(adr.sin_port);

	return 1;
}

/* Examina simultàniament durant "Temps" (en [ms] els sockets (poden ser  */
/* TCP, UDP i stdin) amb identificadors en la llista “LlistaSck” (de      */
/* longitud “LongLlistaSck” sockets) per saber si hi ha arribat alguna    */
/* cosa per ser llegida. Si Temps és -1, s'espera indefinidament fins que */
/* arribi alguna cosa.                                                    */
/* "LlistaSck" és un vector d'enters d'una longitud >= LongLlistaSck      */
/* Retorna -1 si hi ha error; retorna -2 si passa "Temps" sense que       */
/* arribi res; si arriba alguna cosa per algun dels sockets, retorna      */
/* l’identificador d’aquest socket.                                       */
int HaArribatAlgunaCosaEnTemps(const int *LlistaSck, int LongLlistaSck, int Temps)
{
	fd_set fdsel; int i, max=0, ret;

	FD_ZERO(&fdsel);

	for( i=0; i<LongLlistaSck; i++){

	 	FD_SET(LlistaSck[i], &fdsel);

	 	if(LlistaSck[i]>max) max = LlistaSck[i];
	}

	struct timeval time = {0, Temps};

	if(Temps == -1) ret = select(max+1, &fdsel, NULL, NULL, NULL);
	else ret = select(max+1, &fdsel, NULL, NULL, &time);

	if(ret == 0) return -2; else if (ret == -1) return -1;

	i=0;
	while( i<LongLlistaSck && !FD_ISSET(LlistaSck[i] , &fdsel) ) i++;

	if(i<LongLlistaSck) return LlistaSck[i];
	else return -1;
}

/* Donat el nom DNS "NomDNS" obté la corresponent @IP i l'escriu a "IP*"  */
/* "NomDNS" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud qualsevol, i "IP*" és un "string" de C (vector de */
/* chars imprimibles acabat en '\0') d'una longitud màxima de 16 chars    */
/* (incloent '\0').                                                       */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé     */
int ResolDNSaIP(const char *NomDNS, char *IP)
{

	struct hostent *info;
	struct in_addr adrInfo;

	info=gethostbyname(NomDNS);
	if(info==NULL) return -1;

	adrInfo.s_addr=*((unsigned long *)info->h_addr_list[0]);
	strcpy(IP,(char *)inet_ntoa(adrInfo));

	return 1;
}

/* Crea un fitxer de "log" de nom "NomFitxLog".                           */
/* "NomFitxLog" és un "string" de C (vector de chars imprimibles acabat   */
/* en '\0') d'una longitud qualsevol.                                     */
/* Retorna -1 si hi ha error; l'identificador del fitxer creat si tot va  */
/* bé.                                                                    */
int Log_CreaFitx(const char *NomFitxLog)
{
	return fdlog = open (NomFitxLog, O_CREAT|O_WRONLY,0644);
}

/* Escriu al fitxer de "log" d'identificador "FitxLog" el missatge de     */
/* "log" "MissLog".                                                       */
/* "MissLog" és un "string" de C (vector de chars imprimibles acabat      */
/* en '\0') d'una longitud qualsevol.                                     */
/* Retorna -1 si hi ha error; el nombre de caràcters del missatge de      */
/* "log" (sense el '\0') si tot va bé                                     */
int Log_Escriu(int FitxLog, const char *MissLog)
{
	return write(FitxLog, MissLog, strlen(MissLog));
}

/* Tanca el fitxer de "log" d'identificador "FitxLog".                    */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int Log_TancaFitx(int FitxLog)
{
	return close(FitxLog)*2+1;	
}


/* Si ho creieu convenient, feu altres funcions...                        */

