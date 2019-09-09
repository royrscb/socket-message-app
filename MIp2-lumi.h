/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer capçalera de lumi.c                                             */
/*                                                                        */
/* Autors: roy, quim                                                      */
/*                                                                        */
/**************************************************************************/

struct agent {

	char username[101];
	int conectat;
	char IP[16];
	int portUDP;
};

struct agentEsperant{

	char adrMIcli[101];
	char adrMIser[101];

	char IPcli[16];
	int portUDPcli;
};


/* Declaració de funcions externes de lumi.c, és a dir, d'aquelles que es */
/* faran servir en un altre fitxer extern, p.e., MIp2-p2p.c,              */
/* MIp2-nodelumi.c, o MIp2-agelumic.c. El fitxer extern farà un #include  */
/* del fitxer .h a l'inici, i així les funcions seran conegudes en ell.   */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa LUMI.                                  */
/* Les funcions externes les heu de dissenyar vosaltres...                */
int LUMI_CreaSock(const char *IPloc, int portUDPloc);
int LUMI_TrobaAdrSockLoc(int Sck, char *IPloc, int *portUDPloc);
int LUMI_HaArrivatPeti(int Sck, int temps);
int LUMI_HaArrivatAlgo(const int* ll, int lenll, int temps);
int LUMI_RepResp(int Sck, char *IPTCPser, int *portTCPser);
int LUMI_EnviaResp(int Sck, char *IPUDPcli, int portUDPcli, char *IPTCPser, int portTCPser );
int LUMI_RepPeti(int Sck, char *IPUDPcli, int *portUDPcli, char *adrMIser);
int LUMI_EnviaPeti(int Sck, char *IPUDPnode, int portUDPnode, char *adrMIser);
int LUMI_RepAlgo(int Sck, char *IPcli, int *portUDPcli, char *tipus, char *linia);
int LUMI_EnviaAlgo(int Sck, char *IP, int portUDP, char* tipus, char *linia);
int LUMI_tradueixAdr(int Sck, char *linia, char *IPser, int *portTCPser, char *adrcli);
int LUMI_EnviaRespAdr(int Sck, char *IPcli, int portUDPcli, int inf, char *IPser, int portTCPser, char *adrMIcli );
int LUMI_EnviaPetiRegOrDesreg(int Sck, char * ipNode, int portUDPnode, char *adrMI, int reg_desreg);
int LUMI_creaTauAgents(char * nomFix, struct agent *tau, char *domini);
int LUMI_BuscaAtau(struct agent *tau, const int n, char *username);
int LUMI_trobaAdrDeTauOrLog(struct agent *tau, const int n, char *IPLOC, char *adrMI, char *IP, int *portUDP);
void LUMI_separaAdrMI(char *linia, char *adrPreguntada, char *adrPreguntador);
int LUMI_separaUsrDomini(char *adrMI, char *username, char *domini);
