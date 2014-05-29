#include "protocolo.h"
#include "funcoes.h"

mensagem_bin MensagemToMensagem_bin (mensagem msg)
{
	mensagem_bin mbin;
	strcpy (mbin.inicio,"11101110");
	intTobin(msg.tamanho,TAMTAMANHOBIN,mbin.tamanho);
    bzero (mbin.sequencia,TAMSEQUENCIABIN);
	intTobin(msg.tipo, TAMTIPOBIN, mbin.tipo);
	// mbin.dados[0]=msg.dados[0];
	// mbin.dados[1]=msg.dados[1];
	intTobin(msg.dados[0], 8, mbin.dados);
	intTobin(msg.dados[1], 8, &(mbin.dados[8]));
    InsereParidade (&mbin);
	return mbin;
}

mensagem Mensagem_binToMensagem (mensagem_bin msg_bin)
{
	int i;
	mensagem msg;
	msg.tamanho = binToint(msg_bin.tamanho,TAMTAMANHOBIN);
	msg.tipo = binToint(msg_bin.tipo, TAMTIPOBIN);
	for (i = 0; i < msg.tamanho; i++)
	 	msg.dados[i] = binToint(&(msg_bin.dados[i*8]),8);
	// msg.dados[0]=msg_bin.dados[0];
	// msg.dados[1]=msg_bin.dados[1];
	return msg;
}

void envia_mensagem_bin (int socket, mensagem_bin *msg_bin)
{
	if (send (socket, msg_bin, TAMMSG,0)!=TAMMSG)
		puts ("Erro ao enviar mensagem");
}

void recebe_mensagem_bin (int socket, mensagem_bin *msg_bin)
{

	while ( (recv (socket, msg_bin, TAMMSG,0)!=TAMMSG) && (strcmp(msg_bin->inicio, "11101110")!=0));
}

void EnviaArq(int s, char * path, int type)
{
    FILE *fp;
    mensagem msg;
    mensagem_bin msg_bin;
    puts ("\tAbrindo arquivo para envio.");	//log
    if ((fp = fopen (path,"rb"))==NULL)
    {
        printf ("\tArquivo nao existe.\n");	//log
        msg.tipo=ERRO;
        msg.tamanho=0;
        bzero (msg.dados,2);
        msg_bin = MensagemToMensagem_bin(msg);
        envia_mensagem_bin (s, &msg_bin);
        return;
    }

    if ( (type==GET) || (type==PUT) )   // se eh GET ou PUT, envia mensagem com tamanho do arquivo.
    {
	    fseek(fp, 0L, SEEK_END);
	    unsigned int tamfp = ftell(fp);	// fazer barra de progresso
	    int i=0;
	    rewind (fp);
	    msg.tipo = TAMARQ;
	    msg.tamanho = 1;
	    msg.dados[0] = tamfp;
	    msg_bin = MensagemToMensagem_bin(msg);
	    envia_mensagem_bin(s, &msg_bin);
	    printf("\tTamanho do arquivo: %uB.\n", tamfp);
    }

    msg.tipo=MOSTRA;
    msg.tamanho=0;
    bzero (msg.dados,2);
    int i=0;
	puts ("\tInicio da transferencia do arquivo.");	//log
    while (!feof(fp))
    {
        if (msg.tamanho<2)
            fread (&(msg.dados[(msg.tamanho)++]), sizeof(char), 1, fp);
        if( (msg.tamanho == 2) || (feof(fp)) ){
            if (feof(fp))
                msg.tipo=FIMTXT;
            msg_bin = MensagemToMensagem_bin(msg);
            envia_mensagem_bin (s, &msg_bin);
            i++;
            msg.tamanho=0;
            bzero (msg.dados,2);
        }
    }
    printf ("\t[%d mensagens enviadas]\n\tFim da transferencia do arquivo.\n", i);	//log
    fclose(fp); 
}

void InsereParidade (mensagem_bin *msg_bin)
{
    int i; int conta1[4];
    bzero (msg_bin->erro, TAMERROBIN);
    bzero (&conta1, sizeof(int)*4);

    for (i = 0; i < 4; i++)
    {
        if (msg_bin->tamanho[i]==1)      //soma 1's presentes no tamanho
            conta1[0]++;
        if (msg_bin->tipo[i]==1)         //soma 1's presentes no tipo
            conta1[1]++;
        if (msg_bin->sequencia[i]==1)    //soma 1's presentes na sequencia
            conta1[2]++;
        if (msg_bin->dados[i]==1)       //soma 1's presentes nos dados (nao todos)
            conta1[3]++;
    }
    for (i = 0; i < 3; i++)             //insere bit de paridade do tamanho, tipo e sequencia
        if (!EhImpar(conta1[i]))
            msg_bin->erro[i]=1;

    for (i = 4; i<16; i++)              //soma 1's restantes nos dados.
        if (msg_bin->dados[i]==1)
            conta1[3]++;

    if (!EhImpar(conta1[3]))             //insere bit de paridade dos dados
        msg_bin->erro[3] = 1;
}

int TemErro (mensagem_bin msg_bin)
{
    // tamanho dados tipo sequencia
    int i; int conta1[4];
    bzero (&conta1, sizeof(int)*4);

    for (i = 0; i < 4; i++)
    {
        if (msg_bin.erro[i]==1)         // soma bit de erro
            conta1[i]++;
        if (msg_bin.tamanho[i]==1)      //soma 1's presentes no tamanho
            conta1[0]++;
        if (msg_bin.tipo[i]==1)         //soma 1's presentes no tipo
            conta1[1]++;
        if (msg_bin.sequencia[i]==1)    //soma 1's presentes na sequencia
            conta1[2]++;
        if (msg_bin.dados[i]==1)       //soma 1's presentes nos dados (nao todos)
            conta1[3]++;
    }
    for (i = 0; i < 3; i++)
        if (!EhImpar(conta1[i]))         //detecta erro no tamanho, tipo e sequencia
            return 1;

    for (i = 4; i<16; i++)
        if (msg_bin.dados[i]==1)       //soma restante dos 1's
            conta1[3]++;

    if (!EhImpar(conta1[3]))             //detecta erro nos dados
        return 1;
    return 0;                           //nao tem erro
}