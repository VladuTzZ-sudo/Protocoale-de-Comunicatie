Matei Vlad Cristian 
321CC
                    TEMA PROTOCOALE DE COMUNICATIE
Tema 2 - Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

                            ==  SUBSCRIBER  ==

    :: Subscriber-ul va comunica cu server-ul, el reprezentand un client TCP.
    :: Am dezactivat buffering-ul la afisare prin comanda 
                    "" setvbuf(stdout, NULL, _IONBF, BUFSIZ) "" 
    :: Am declarat multimile de descriptori: read_fds, tmp_fds unde tmp_fds 
    reprezinta multimea read_fds temporara, pentru si dupa apelarea functiei
    select() care modifica multimea.
    :: Se sterge in intregime multimea de descriptori de fisiere read_fds | tmp_fds.
    :: Se verifica ca primim un numar minim de 4 argumente. (exec + ID + IP + port).
    :: Se salveaza port-ul pe care se va conecta subscriber-ul.
    :: Se foloseste functia socket() pentru a obtine descriptorul de fisier
	corespunzator unei conexiuni TCP.
    :: Se configureaza datele server-ului.
    :: Se incearca conectarea pe acest acest server.
    :: Se adauga in multimea descriptorilor de fisiere pentru citire, socket-ul
    server-ului si STDIN ,deoarece client-ul poate primi date doar de la socket si 
    STDIN.
    :: Se selecteaza descriptorul de fisiere pentru care se primesc date.
    :: Se testeaza daca descriptorul i apartine sau nu multimii tmp_fds.
    :1: In caz ca se primesc date de la STDIN, se verifica daca comanda 
    primita este "exit", in caz afirmativ se deconecteaza acest client.
    :: In caz ca mesajul este de tip "subscribe", se construieste mesajul de
    tip TCP, prin identificarea elementelor din buffer.
        -> Se incearca dezactivarea algoritmului lui Neagle.
        -> Se trimite mesajul catre server.
    :: In caz ca mesajul este de tip "unsubscribe", se construieste mesajul 
    de tip TCP, analog.
        -> Se incearca dezactivarea algoritmului lui Neagle.
        -> Se trimite mesajul catre server.
    :: Daca se trimite o comanda diferita de "subscribe" | "unsubscribe" se
    printeaza "Invalid command !".
    :2: In caz ca se primesc date de la server acesta trebuie sa fie un mesaj
    de tip UDP.
    :: Daca ce se primeste prin "recv" == 0 => trebuie inchis clientul.
    :: Daca topic-ul mesajului este "inUse" inseamna ca astfel server-ul anunta 
    clientul ca deja exista un client cu acest ID conectat. => se inchide.
    :: Identific tipul de mesaj si afisez corespunzator.

                            ==   SERVER   ==

    :: Am dezactivat buffering-ul la afisare prin comanda 
                    "" setvbuf(stdout, NULL, _IONBF, BUFSIZ) "" 
    :: Se declara multimile de descriptori. 
    :: Se sterge in intregime multimea de descriptori de fisiere read_fds | tmp_fds.
    :: Se foloseste functia socket() pentru a obtine descriptorii de fisiere
	corespunzatori unei conexiuni UDP si TCP.
    :: Se incearca dezactivarea algoritmului lui Neagle pentru socketi.
    :: Se configureaza datele server-ului, se face bind pentru socketi.
    :: Socket-ul pe care se poate asculta este socket-ul corespunzator conexiunii TCP.
    :: Se adauga in multimea read_fds descriptorii de fisiere.
    :: Se selecteaza descriptorul de fisiere pentru care se primesc date.
        -> i == STDIN_FILENO  << citirea de la tastatura.
        -> i == socketUDP     << mesaje de tip UDP venite pe acest socket.
        -> i == socketTCP     << mesaje de tip TCP venite pe acest socket.
    
    :: EXPLICAREA STRUCUTRILOR. 
                            ==   HELPERS   ==

        struct messageUDP                   struct messageTCP
        ""                                  ""
	        char topic[50];                     int type;
	        char typeDate;                      char topic[50];
	        char buffer[1500];                  int SF;        ""
	        char ip[20];
	        int port;   ""                  struct ListClients
                                            ""
        struct ClientTCP                        ClientTCP *client;
        ""                                      int SF;
	        char ID[10];                        ListClients *next; ""
	        int connected;                  
	        int socket;                         struct Topics
	        int messagesLength;                 ""  char name[50];
	        messageUDP *waitingMessages; ""         ListClients *clients; ""

    :: messageUDP   -> structura pentru a reprezenta un mesaj provenit de la un
    client UDP prin protocolul UDP.
    :: messageTCP   -> structura pentru a reprezenta un mesaj provenit de la un
    client TCP prin protocolul TCP.
    :: ClientTCP    -> reprezinta un client, messagesLength = lungimea mesajelor
    pe care le are in asteptare de la canalele la care este abonat si care s-au 
    strans cat timp a fost offline. waitingMessages = mesajele.
    :: ListClients  -> lista de clienti care se va regasi in server, unde exista o
    lista principala de clienti connectati. Totodata in in fiecare topic se va afla
    o lista de clienti care sunt abonati canalului. Atunci cand lista este folosita
    in interiorul unui topic, campul SF = capata sens.
    :: Topics       -> structura folosita pentru a stoca in server un vector de 
    topic-uri, unde fiecare topic are un nume si o lista de clienti.    

    :: In caz ca se primesc date pe socket-ul corespunzator conexiunii UDP,
    se accepta mesajul, se identifica elementele si se creeaza mesajul de tip UDP.
    :: In caz ce nu exista topic-uri in vectorul de topics, Se initializeaza 
    vectorul de topics cu topic-ul acestui mesaj ca prim topic.
    :: In caz ca exista cel putin 1 topic in vectorul de topics se apeleaza functia.
    sendToClient:
        -> Functia are ca scop sa gaseasca pentru topic-ul mesajului lista de 
        clienti abonati si sa le transmita mesajul sau cel putin sa puna mesajul
        in asteptare pana cand se vor conecta. 
        -> Daca s-a gasit topic-ul cautat in vectorul de topics, atunci se parcurg
        clientii. Daca exista clienti, se transmite la cei conectati mesajul, daca
        sunt offline si au SF == 1, se pune mesajul in asteptare in 
        structura aferenta clientului, altfel nu.
        !!!! -> Daca nu s-a gasit topic-ul cautat atunci se creeaza 
        acum acest topic si se adauga in vectorul de topics.
    :: In caz ca se doreste o noua conexiune TCP, Se adauga socket-ul clientului 
    nou in multimea de descriptori de fisiere. Pentru acest socket se poate primi
    informatie doar de la clinetii TCP care exista sau vor lua pentru prima oara
    contact cu server-ul.
    :: Se adauga socket-ul clientului nou in multimea de descriptori de fisiere.
    :: Se incearca dezactivarea algoritmului lui Neagle.
    :: Se adauga clientul nou in lista de clienti, doar daca este un client nou.
    :: Se incearca parcurgerea listei de clienti deja conectati, 
    adaugarea se face la sfarsit.
    :: Daca clientul nou s-a gasit in lista de clienti inseamna ca a mai fost 
    conectat si a iesit ,dar acum se reconecteaza sau chiar acum este conectat.
    :: Daca chiar acum este conectat trimitem catre client un mesaj "inUse"
	ce va fi decodificat in client si printeaza in server (aici) eroarea aferenta.
    :: Daca un client care se reconecteaza la server se reactualizeaza socket-ul.                        
        -> se cauta in listele de clienti abonati la topic-uri daca si acesta este
	    abonat pentru a i se trimite mesajele primite cat a fost offline, 
        daca SF == 1.
    :: Daca s-a ajuns la sfarsitul iterarii si nu s-a gasit clientul, 
    atunci se adauga ca unul nou.
    :: Se printeaza in server mesajul asteptat pentru o noua conexiune 
    cu un client TCP.
    
    :: Daca s-a primit informatie pe un alt socket, inseamna ca este un socket
    al unui client TCP si se interpreteaza datele.
    :: Daca n == 0, atunci inseamna ca acest client trebuie deconectat de la server.
    :: Altfel, se verifica ce tip de mesaj se primeste SUBSCRIBE | UNSUBSCRIBE.
    :: Daca nu exista niciun topic inregistrat in server,
    atunci se poate doar subscribe.
        -> Se creeaza aici primul topic din server, initializandu-se cu primul client.
    :: Altfel, se cauta printre topic-urile din server daca cel la care 
    vrea clientul sa se aboneze exista. 
        -> Daca topic-ul exista, atunci se realizeaza operatia corespunzatoare.
    :: Daca topic-ul cautat nu este gasit, atunci se reinitializeaza 
    vectorul de topics. Topic-ul nou adaugat va avea ca prim abonat acest client.
    :: Se inchid sockets.                        
	                        
                                
	                            