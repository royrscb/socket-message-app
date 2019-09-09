/**************************************************************************/
/*                                                                        */
/* P1 - MI amb sockets TCP/IP - Part I                                    */
/* Fitxer mi.c que implementa la capa d'aplicació de MI, sobre la capa de */
/* transport TCP (fent crides a la interfície de la capa TCP -sockets-).  */
/* Autors: X, Y                                                           */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <sys/types.h> o #include "meu.h" */
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
#include "MIp2-lumi.h"
/* Definició de constants, p.e., #define MAX_LINIA 150                    */

/* Declaració de funcions internes que es fan servir en aquest fitxer     */
/* (les seves definicions es troben més avall) per així fer-les conegudes */
/* des d'aqui fins al final de fitxer.                                    */
int TCP_CreaSockClient(const char *IPloc, int portTCPloc);
int TCP_CreaSockServidor(const char *IPloc, int portTCPloc);
int TCP_DemanaConnexio(int Sck, const char *IPrem, int portTCPrem);
int TCP_AcceptaConnexio(int Sck, char *IPrem, int *portTCPrem);
int TCP_Envia(int Sck, const char *SeqBytes, int LongSeqBytes);
int TCP_Rep(int Sck, char *SeqBytes, int LongSeqBytes);
int TCP_TancaSock(int Sck);
int TCP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portTCPloc);
int TCP_TrobaAdrSockRem(int Sck, char *IPrem, int *portTCPrem);
int HaArribatAlgunaCosa(const int *LlistaSck, int LongLlistaSck);


MI_getPortLoc(int fdsoc, int *portTCPloc){

	char res[16];

	return TCP_TrobaAdrSockLoc(fdsoc, res, portTCPloc);
}

int MI_getIPloc(char *IPloc){


	struct ifaddrs *ifa; struct sockaddr_in *sa; getifaddrs (&ifa);

	for (; ifa; ifa = ifa->ifa_next) 
		if (ifa->ifa_addr->sa_family==AF_INET) {
	   		sa = (struct sockaddr_in *) ifa->ifa_addr;

	   		if( strncmp(inet_ntoa(sa->sin_addr), "127.0.0.1", 9) != 0 ) strcpy(IPloc,inet_ntoa(sa->sin_addr));	        
	}

		freeifaddrs(ifa);

	return 1;
}


/* Definicio de funcions EXTERNES, és a dir, d'aquelles que en altres     */
/* fitxers externs es faran servir.                                       */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa MI.                                    */

/* Inicia l’escolta de peticions remotes de conversa a través d’un nou    */
/* socket TCP en el #port “portTCPloc” i una @IP local qualsevol (és a    */
/* dir, crea un socket “servidor” o en estat d’escolta – listen –).       */
/* Retorna -1 si hi ha error; l’identificador del socket d’escolta de MI  */
/* creat si tot va bé.                                                    */
int MI_IniciaEscPetiRemConv(int portTCPloc){

	return TCP_CreaSockServidor("0.0.0.0", portTCPloc);
}

/* Escolta indefinidament fins que arriba una petició local de conversa   */
/* a través del teclat o bé una petició remota de conversa a través del   */
/* socket d’escolta de MI d’identificador “SckEscMI” (un socket           */
/* “servidor”).                                                           */
/* Retorna -1 si hi ha error; 0 si arriba una petició local; SckEscMI si  */
/* arriba una petició remota.                                             */
int MI_HaArribatPetiConv(int SckEscMI){

	fd_set fdsel;
	FD_ZERO(&fdsel);
	FD_SET(0, &fdsel); FD_SET(SckEscMI, &fdsel);

	if(select(SckEscMI+1, &fdsel, NULL, NULL, NULL) == -1) return -1;

	if(FD_ISSET(0, &fdsel)) return 0;
	if(FD_ISSET(SckEscMI, &fdsel)) return SckEscMI;
}

/* Crea una conversa iniciada per una petició local que arriba a través   */
/* del teclat: crea un socket TCP “client” (en un #port i @IP local       */
/* qualsevol), a través del qual fa una petició de conversa a un procés   */
/* remot, el qual les escolta a través del socket TCP ("servidor") d'@IP  */
/* “IPrem” i #port “portTCPrem” (és a dir, crea un socket “connectat” o   */
/* en estat establert – established –). Aquest socket serà el que es farà */
/* servir durant la conversa.                                             */
/* Omple “IPloc*” i “portTCPloc*” amb, respectivament, l’@IP i el #port   */
/* TCP del socket del procés local.                                       */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem*” amb el nickname remot. El procés */
/* local és qui inicia aquest intercanvi (és a dir, primer s’envia el     */
/* nickname local i després es rep el nickname remot).                    */
/* "IPrem" i "IPloc*" són "strings" de C (vectors de chars imprimibles    */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "NicLoc" i "NicRem*" són "strings" de C (vectors de chars imprimibles  */
/* acabats en '\0') d'una longitud màxima de 300 chars (incloent '\0').   */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa de   */
/* MI creat si tot va bé.                                                 */
int MI_DemanaConv(const char *IPrem, int portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem){

	int fdsoc; 

	if( (fdsoc = TCP_CreaSockClient(IPloc, *portTCPloc)) == -1 ) return -1;
	if( TCP_DemanaConnexio(fdsoc, IPrem, portTCPrem) == -1 ) return -1;

	TCP_TrobaAdrSockLoc(fdsoc, IPloc, portTCPloc);

	TCP_enviaNick(fdsoc, NicLoc);
	TCP_repNick(fdsoc, NicRem);
	return fdsoc;	
}

/* Crea una conversa iniciada per una petició remota que arriba a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”): accepta la petició i crea un socket (un socket            */
/* “connectat” o en estat establert – established –), que serà el que es  */
/* farà servir durant la conversa.                                        */
/* Omple “IPrem*”, “portTCPrem*”, “IPloc*” i “portTCPloc*” amb,           */
/* respectivament, l’@IP i el #port TCP del socket del procés remot i del */
/* socket del procés local.                                               */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem*” amb el nickname remot. El procés */
/* remot és qui inicia aquest intercanvi (és a dir, primer es rep el      */
/* nickname remot i després s’envia el nickname local).                   */
/* "IPrem*" i "IPloc*" són "strings" de C (vectors de chars imprimibles   */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "NicLoc" i "NicRem*" són "strings" de C (vectors de chars imprimibles  */
/* acabats en '\0') d'una longitud màxima de 300 chars (incloent '\0').   */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa      */
/* de MI creat si tot va bé.                                              */
int MI_AcceptaConv(int SckEscMI, char *IPrem, int *portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem){

	int fdcx;

	if( (fdcx=TCP_AcceptaConnexio(SckEscMI, IPrem, portTCPrem)) == -1) return -1;
	if(TCP_TrobaAdrSockLoc(fdcx, IPloc, portTCPloc) == -1) return -1;
	
	TCP_repNick(fdcx, NicRem);
	TCP_enviaNick(fdcx, NicLoc);
	
	return fdcx;
}

/* Escolta indefinidament fins que arriba una línia local de conversa a   */
/* través del teclat o bé una línia remota de conversa a través del       */
/* socket de conversa de MI d’identificador “SckConvMI” (un socket        */
/* "connectat”).                                                          */
/* Retorna -1 si hi ha error; 0 si arriba una línia local; SckConvMI si   */
/* arriba una línia remota.                                               */
int MI_HaArribatLinia(int SckConvMI){

	fd_set fdsel;
	FD_ZERO(&fdsel);
	FD_SET(0, &fdsel); FD_SET(SckConvMI, &fdsel);

	if(select(SckConvMI+1, &fdsel, NULL, NULL, NULL) == -1) return -1;

	if(FD_ISSET(0, &fdsel)) return 0;
	if(FD_ISSET(SckConvMI, &fdsel)) return SckConvMI;
}

/* Envia a través del socket de conversa de MI d’identificador            */
/* “SckConvMI” (un socket “connectat”) la línia “Linia” escrita per       */
/* l’usuari local.                                                        */
/* "Linia" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0'), no conté el caràcter fi de línia ('\n') i té una longitud       */
/* màxima de 300 chars (incloent '\0').                                   */
/* Retorna -1 si hi ha error; el nombre de caràcters n de la línia        */
/* enviada (sense el ‘\0’) si tot va bé (0 <= n <= 299).                  */
int MI_EnviaLinia(int SckConvMI, const char *Linia){

	char msg[303]; msg[0]='L';

	int lentxt = strlen(Linia)-1; 

	snprintf(msg+1, 4, "%03d", lentxt);  //afegeix llargaria
	strncpy(msg+4, Linia, lentxt);   //i afegeix missatge

	TCP_Envia(SckConvMI, msg, lentxt+4);            

	return lentxt;
	}


/* Rep a través del socket de conversa de MI d’identificador “SckConvMI”  */
/* (un socket “connectat”) una línia escrita per l’usuari remot, amb la   */
/* qual omple “Linia”, o bé detecta l’acabament de la conversa per part   */
/* de l’usuari remot.                                                     */
/* "Linia*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0'), no conté el caràcter fi de línia ('\n') i té una longitud       */
/* màxima de 300 chars (incloent '\0').                                   */
/* Retorna -1 si hi ha error; -2 si l’usuari remot acaba la conversa; el  */
/* nombre de caràcters n de la línia rebuda (sense el ‘\0’) si tot va bé  */
/* (0 <= n <= 299).                                                       */
int MI_RepLinia(int SckConvMI, char *Linia){

	if( TCP_Rep(SckConvMI, Linia, 1) == 0) return-2;
	else if(Linia[0]=='L'){

		TCP_Rep(SckConvMI, Linia, 3); Linia[4]='\0';
		int lentxt = atoi(Linia);

		TCP_Rep(SckConvMI, Linia, lentxt);

		Linia[lentxt]='\0';
		
		return lentxt;
	}

	return -1;
}

/* Acaba la conversa associada al socket de conversa de MI                */
/* d’identificador “SckConvMI” (un socket “connectat”).                   */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaConv(int SckConvMI){

	return (close(SckConvMI)*2)+1;
}

/* Acaba l’escolta de peticions remotes de conversa que arriben a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”).                                                           */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaEscPetiRemConv(int SckEscMI){

	return (close(SckEscMI)*2)+1;
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* Definicio de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer.                                  */


/* Crea un socket TCP “client” a l’@IP “IPloc” i #port TCP “portTCPloc”   */
/* (si “IPloc” és “0.0.0.0” i/o “portTCPloc” és 0 es fa/farà una          */
/* assignació implícita de l’@IP i/o del #port TCP, respectivament).      */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int TCP_CreaSockClient(const char *IPloc, int portTCPloc){

	struct sockaddr_in adr;
	int fdsoc;

	adr.sin_family=AF_INET;
	adr.sin_port=htons(portTCPloc);
	adr.sin_addr.s_addr=inet_addr(IPloc);
	bzero(&(adr.sin_zero),8);

	if((fdsoc=socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;
	if(bind(fdsoc, (struct sockaddr*) &adr, sizeof(adr)) == -1) return -1;

	return fdsoc;
}

/* Crea un socket TCP “servidor” (o en estat d’escolta – listen –) a      */
/* l’@IP “IPloc” i #port TCP “portTCPloc” (si “IPloc” és “0.0.0.0” i/o    */
/* “portTCPloc” és 0 es fa una assignació implícita de l’@IP i/o del      */
/* #port TCP, respectivament).                                            */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int TCP_CreaSockServidor(const char *IPloc, int portTCPloc){

	struct sockaddr_in adr;		int fdsoc;

	adr.sin_family=AF_INET;
	adr.sin_port=htons(portTCPloc);
	adr.sin_addr.s_addr=inet_addr(IPloc);
	bzero(&(adr.sin_zero),8);

	if( (fdsoc=socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;
	if(bind(fdsoc, (struct sockaddr*) &adr, sizeof(adr)) == -1) return -1;
	if(listen(fdsoc,1) == -1) return -1;
	
	return fdsoc;
}

/* El socket TCP “client” d’identificador “Sck” demana una connexió al    */
/* socket TCP “servidor” d’@IP “IPrem” i #port TCP “portTCPrem” (si tot   */
/* va bé es diu que el socket “Sck” passa a l’estat “connectat” o         */
/* establert – established –).                                            */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_DemanaConnexio(int Sck, const char *IPrem, int portTCPrem){

	struct sockaddr_in adrem;
	
	adrem.sin_family=AF_INET;
	adrem.sin_addr.s_addr=inet_addr(IPrem); 
	adrem.sin_port=htons(portTCPrem);
	bzero(&(adrem.sin_zero),8);

	if( (connect(Sck, (struct sockaddr*) &adrem, sizeof(adrem))) == -1 ) return -1;

	return 1;
}

/* El socket TCP “servidor” d’identificador “Sck” accepta fer una         */
/* connexió amb un socket TCP “client” remot, i crea un “nou” socket,     */
/* que és el que es farà servir per enviar i rebre dades a través de la   */
/* connexió (es diu que aquest nou socket es troba en l’estat “connectat” */
/* o establert – established –; el nou socket té la mateixa adreça que    */
/* “Sck”).                                                                */
/* Omple “IPrem*” i “portTCPrem*” amb respectivament, l’@IP i el #port    */
/* TCP del socket remot amb qui s’ha establert la connexió.               */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket connectat creat  */
/* si tot va bé.                                                          */
int TCP_AcceptaConnexio(int Sck, char *IPrem, int *portTCPrem){

	struct sockaddr_in adrem; int fdcx, len=sizeof(adrem);	

	if( (fdcx=accept(Sck,(struct sockaddr *) &adrem, &len))==-1) return -1;

	if(TCP_TrobaAdrSockRem(fdcx, IPrem, portTCPrem)==-1) return -1;

	return fdcx;
}

/* Envia a través del socket TCP “connectat” d’identificador “Sck” la     */
/* seqüència de bytes escrita a “SeqBytes” (de longitud “LongSeqBytes”    */
/* bytes) cap al socket TCP remot amb qui està connectat.                 */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud >= LongSeqBytes bytes.      */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int TCP_Envia(int Sck, const char *SeqBytes, int LongSeqBytes){

	return write(Sck, SeqBytes, LongSeqBytes);
}

/* Rep a través del socket TCP “connectat” d’identificador “Sck” una      */
/* seqüència de bytes que prové del socket remot amb qui està connectat,  */
/* i l’escriu a “SeqBytes*” (que té una longitud de “LongSeqBytes” bytes),*/
/* o bé detecta que la connexió amb el socket remot ha estat tancada.     */
/* "SeqBytes*" és un vector de chars qualsevol (recordeu que en C, un     */
/* char és un enter de 8 bits) d'una longitud <= LongSeqBytes bytes.      */
/* Retorna -1 si hi ha error; 0 si la connexió està tancada; el nombre de */
/* bytes rebuts si tot va bé.                                             */
int TCP_Rep(int Sck, char *SeqBytes, int LongSeqBytes){

	return read(Sck, SeqBytes, LongSeqBytes);
}

/* S’allibera (s’esborra) el socket TCP d’identificador “Sck”; si “Sck”   */
/* està connectat es tanca la connexió TCP que té establerta.             */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TancaSock(int Sck){

	return (close(Sck)*2)+1;
}

/* Donat el socket TCP d’identificador “Sck”, troba l’adreça d’aquest     */
/* socket, omplint “IPloc*” i “portTCPloc*” amb respectivament, la seva   */
/* @IP i #port TCP.                                                       */
/* "IPloc*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portTCPloc){

	struct sockaddr_in adr;
	int len = sizeof(adr);

	if( getsockname(Sck, (struct sockaddr*) &adr,&len) == -1) return -1;

	strcpy(IPloc,inet_ntoa(adr.sin_addr)); 
	*portTCPloc=ntohs(adr.sin_port);

	return 1;
}

/* Donat el socket TCP “connectat” d’identificador “Sck”, troba l’adreça  */
/* del socket remot amb qui està connectat, omplint “IPrem*” i            */
/* “portTCPrem*” amb respectivament, la seva @IP i #port TCP.             */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TrobaAdrSockRem(int Sck, char *IPrem, int *portTCPrem){

	struct sockaddr_in adrem;
	int len = sizeof(adrem), port;

	if( getpeername(Sck, (struct sockaddr*) &adrem,&len) == -1) return -1;
	strcpy(IPrem,inet_ntoa(adrem.sin_addr)); 
	*portTCPrem=ntohs(adrem.sin_port); 

	return 1;
}

/* Examina simultàniament i sense límit de temps (una espera indefinida)  */
/* els sockets (poden ser TCP, UDP i stdin) amb identificadors en la      */
/* llista “LlistaSck” (de longitud “LongLlistaSck” sockets) per saber si  */
/* hi ha arribat alguna cosa per ser llegida.                             */
/* "LlistaSck" és un vector d'enters d'una longitud >= LongLlistaSck      */
/* Retorna -1 si hi ha error; si arriba alguna cosa per algun dels        */
/* sockets, retorna l’identificador d’aquest socket.                      */
int HaArribatAlgunaCosa(const int *LlistaSck, int LongLlistaSck)
{
	fd_set fdsel; int i, max=0;
	FD_ZERO(&fdsel);

	for( i=0; i<LongLlistaSck; i++){

	 	FD_SET(LlistaSck[i], &fdsel);

	 	if(LlistaSck[i]>max) max = LlistaSck[i];
	}

	if(select(max+1, &fdsel, NULL, NULL, NULL) == -1) return -1;

	i=0;
	while(  i<LongLlistaSck && !FD_ISSET(LlistaSck[i] , &fdsel)  ) i++;

	if(i<LongLlistaSck) return LlistaSck[i];
	else return -1;
}

/* Envia el nicknames amb la part servidora
retorna -1 en cas d'error i un nombre positiu cualsevol en cas contrari.*/
int TCP_enviaNick(int SckConvMI, const char *NicLoc){

	char nick[303]; nick[0]='N';

	int lentxt = strlen(NicLoc);

	snprintf(nick+1, 4, "%03d", lentxt);  
	strncpy(nick+4, NicLoc, lentxt);   

	if( TCP_Envia(SckConvMI, nick, lentxt+4) == -1) return -1; 
	return 1;
}

/*Rep el nicnkames amb la part client
retorna -1 en cas d'error i un nombre positiu cualsevol en cas contrari.*/
int TCP_repNick(int SckConvMI, char *NickRem){

	if( TCP_Rep(SckConvMI, NickRem, 1) == -1) return -1; 
	if( NickRem[0]!='N') return -1;

	if( TCP_Rep(SckConvMI, NickRem, 3) == -1) return -1;  
	NickRem[4]='\0';
	int lentxt = atoi(NickRem);

	TCP_Rep(SckConvMI, NickRem, lentxt);
	NickRem[lentxt]='\0';
	
	return 1;
}

