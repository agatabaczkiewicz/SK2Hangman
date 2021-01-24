# SK2Hangman

**Sposób kompilacji**

Preferowane jest, aby serwer uruchomić na systemie Linux a klientów na systemie Windows. W przypadku wirtualnej maszyny, może ona sobie nie dać rady z utrzymaniem serwera i kilku klientów równocześnie.
W tym celu konieczna jest edycja pliku configclient.txt na adres lokalny wirtualnej maszyny. Wymagane oczywiście połączenie między VM a Windowsem.  
  
**Serwer:**  

Pliki config.txt i word.txt muszą znajdować się w tym samym folderze co plik server.cpp. Aby skompilować należy w terminalu przejść do folderu z projektem, a następnie wpisać komendę: make server

**Klient:**

Plik configclient.txt, bg-photo.png oraz folder images  muszą się znajdować w tym samym folderze co plik main.cpp.   
Aby skompilować należy w terminalu przejść do folderu z projektem, a następnie wpisać komendę:  
-> jeżeli pracuje się na Linuxie: make client,  
-> jeżeli pracuje się na Windowsie : python main.py  




**Pliki**

server.cpp - plik zawierający serwer, który tworzy i kontroluje grę i komunikacje pomiędzy klientów  
config.txt - plik konfiguracyjny dla serwera. Zawiera: numer portu, nazwę pliku z hasłami i ilość słów  
word.txt - plik z hasłami  
main.py - plik zawierający kod tworzący program dla klienta. Tworzy GUI gry oraz komunikuje się z serwerem  
configclient.txt - plik konfiguracyjny dla klienta. Zawiera: adres ip serwera i numer portu  
komunikaty - plik zawierający opis komunikat, co oznaczają komunikaty jakie serwer przesyła do klienta i klient do serwera

