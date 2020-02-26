# Messaging-Management-App

  Descriere Generala:
  
    --> Severul (unic) - va realiza legătura intre clientii din platformă, cu scopul publicării si abonării la mesaje.
    
    --> Clientii TCP - vor avea următorul comportament: un client TCP se conectează la server, poate primi (in orice moment)
    de la tastatură(interactiunea cu utilizatorul uman) comenzi de tipul subscribe si unsubscribe, afisand pe ecran mesajele
    primite de la server.
    
    --> Clientii UDP - vor trimite catre server mesaje in platforma propusa folosind un protocol predefinit.
    
    
 1. Servarul:
 
  Serverul va avea rolul de broker (componentă de intermediere) in platforma de gestionare a mesajelor. Acesta
va deschide 2 socketi (unul TCP si unul UDP) pe un port primit ca parametru si va astepta conexiuni/datagrame pe toate adresele
IP disponibile local. Pornirea serverului se va face folosind comanda:

    ./server <PORT_DORIT>
    
  Serverul va accepta, de la tastatură, doar comanda exit ce va avea ca efect inchiderea simultană a serverului si a tuturor
clientilor TCP conectati in acel moment.

  Comunicarea cu clientii UDP.
  Pentru comunicare, se vor folosi mesaje ce respectă un format definit:
  

                 |        Topic                     |          Tip Date                |      Continut
    --------------------------------------------------------------------------------------------------------------------
    Dimensiune:  |  Fix 50 de bytes                 |           1 octet                |    Maxim 1500 de octeti
    --------------------------------------------------------------------------------------------------------------------
    Format:      |   Sir de maxim 50 de             | unsinged int pe 1 octet folosit  |   Variabil in functie de tipul
                 | caractere, terminat cu \0 pentru |     pentru a specifica tipul de  |             de date.
                 | dimensiuni mai mici de 50.       |      date al continutului.       |


 2. Clientii TCP:
 
 Clientii de TCP pot fi in orice număr, la fel ca cei UDP, si vor fi rulati folosind comanda următoare
 
    ./subscriber <ID_Client> <IP_Server> <Port_Server>
    • ID_Client este un sir de caractere ce reprezintă ID-ul clientului.
    • IP_Server reprezintă adresa IPv4 a serverului reprezentată folosind notatia dotted-decimal.
    • Port_Server reprezintă portul pe care serverul asteaptă conexiuni. 
  
  Clientii de TCP pot primi de la tastatură una dintre următoarele comenzi:
  
    • subscribe - anuntă serverul că un client este interesat de un anumit topic; comanda are următorul format: 
    "subscribe topic SF" , unde:
          – topic reprezintă topicul la care clientul urmează să se aboneze;
          – SF poate avea valoarea 0 sau 1(1 - vrea sa primeasca mesajele aparute cand era offline; 0 altfel).
          
    • unsubscribe - anuntă serverul că un client nu mai este interesat de un anumit topic; comanda are următorul format:
    "unsubcribe topic" , unde topic reprezintă topicul de la care clientul urmează să se aboneze;
    
    • "exit" - comanda va fi folosită pentru a realiza ı̂nchiderea clientului.
  
  
  3. Clientii UDP:
  
  Pentru a-l rula este nevoie de Python 3.x. Nu sunt necesare biblioteci 3rd-party.
  
  Următoarele sunt câteva exemple de comenzi valide cu care se poate face rularea unui client, dacă serverul rulează local
 pe portul 8080:
 
    • python3 udp_client.py 127.0.0.1 8080
    • python3 udp_client.py --mode manual 127.0.0.1 8080
    • python3 udp_client.py --source-port 1234 --input_file three_topics_payloads.json --mode random --delay 2000 127.0.0.1 8080
  
  În arhiva cu clientul se găsesc și 2 fișiere de input:
  
    • sample_payloads.json - conține mesaje trimise cu topicuri sugestive;
    • three_topics_payloads.json - conține o parte din conținuturile din sample_payloads.json. trimise pe doar 3 topicuri.
    
    
  4. Functionarea aplicatiei:
  
  Initializarea aplicatiei este dată de pornirea serverului, la care ulterior se vor putea conecta un număr variabil de clienti
 TCP / UDP. Se cere ca serverul să permită conectarea/deconectarea de (noi) clienti la orice moment.
 
  Fiecare client va fi identificat prin 'client_id-ul' cu care acesta a fost pornit. La un moment dat, nu vor exista 2 clienti
 conectati cu acelasi ID.
 
  ID-ul unui client va fi un sir de maxim 10 caractere ASCII.
  
  Serverul trebuie să tină evidenta topic-urilor la care este abonat fiecare dintre clienti. La primirea unui mesaj UDP valid,
 serverul trebuie să asigure trimiterea acestuia către toti clientii TCP care sunt abonati la topicul respectiv.
      
