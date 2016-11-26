//Tufa Adriana 333 CA
Tema 3

	Procesarea unui singur fisier
	PPU-ul este cel care citeste datele pentru fisierul dat in linia de comanda
si le pune intr-o structura de tipul file_op. Se verifica daca numarul de spu-uri
dat ca argument este prea mare pentru dimensiunea fisierului si se modifica daca
este cazul. Fiecare SPU va avea de procesat minim 16 octeti. Dupa citirea din
fisierele de intrare, se initializeaza cate o structura pentru fiecare SPE, din care
acesta va citi date. In structura se tin pointeri catre locatiile de memorie in care
se citeste fisierul de intrare, cheia si de unde se scrie in fisierul de iesire. De
asemenea, fiecare SPU isi stie pozitia de la care va citi date din buffer-ul de intrare
si cati octeti va citi. El va pune datele procesate la acelasi offset in bufferul de
iesire.
	Un SPE va lua prin argp adresa structurii alocate pentru el in PPU si va citi din
ea informatiile despre modul de procesare si adresele bufferelor. Apoi calculeaza 
dimensiunea unui transfer: daca dimensiunea datelor de procesat este mai mica decat 16K,
un singur transfer va fi de ajuns; daca nu, se vor face mai multe transferuri de 16K.
Pentru fiecare transfer, se citesc transfer_size octeti din memoria PPU-ului, prin DMA.
Apoi se proceseaza octetii, efectuandu-se encrypt sau decrypt. Am folosit functiile date
in programul serial pentru cele doua operatii. Dupa calculare, se pune rezultatul in
bufferul de iesire din PPU, printr-un apel DMA de tip put. Pentru ca am implementat si
procesarea mai multor fisiere, la terminarea calculelor pentru un fisier, fiecare SPU
trimite prin mailbox un mesaj ce semnifica ca a terminat si asteapta un raspuns: daca mai
sunt fisiere de procesat sau nu. In acest caz nu vor mai fi, de aceea PPU-ul trebuie sa 
anunte SPU-urile de acest lucru prin mailbox.
	Dupa schimbul de mesaje, PPU-ul va scrie datele rezultate in fisierul de output.

	OPERATII VECTORIALE
	Operatiile vectoriale pentru criptare/decriptare se fac pe SPU in functiile
encrypt_block_vect si decrypt_block_vect. Aici se proceseaza un transfer intreg de date.
Bufferul si cheia luate din PPU se vectorizeaza si astfel o pozitie din vector va avea
4 elemente de tipul unsigne int din vectorul initial. Fie v array-ul initial, vect
array-ul de tip vector unsigned int si k cheia. Pentru primele 4 elemente, la criptare, 
vom avea urmatoarele operatii (corepsunzatoare unui element din vect):
	v[0] += ((v[1]<<4) + k[0]) ^ (v[1] + sum) ^ ((v[1]>>5) + k[1]);
	v[1] += ((v[0]<<4) + k[2]) ^ (v[0] + sum) ^ ((v[0]>>5) + k[3]);
	v[2] += ((v[3]<<4) + k[0]) ^ (v[3] + sum) ^ ((v[3]>>5) + k[1]);
	v[3] += ((v[2]<<4) + k[2]) ^ (v[2] + sum) ^ ((v[2]>>5) + k[3]);
Pentru a face operatii vectoriale m-am folosit de urmatoarele variabile ajutatoare:
 v[1]
 v[0]
 v[3]  = b,  a = b << 4, c = b >>5
 v[2]

 k[0]              k[1]
 k[2]              k[3]
 k[0]  = k1        k[1] = k2
 k[2]              k[3]

 k1 si k2 le-am obtinut prin rearanjarea octetilor din key, prin spu_shuffle.
 b l-am obtinut prin rearanjarea octetilor din vect[0].
 a este b << 4 obtinut prin spu_sl.
 c este b >> 5 si este obtinut prin spu_rlmask care roteste la stanga fiecare element si aplica o
 masca pe biti pentru a obtine un shift right. Un unsigned int va fi rotit cu sizeof(unsigned int) - 5
 iar ultimii 5 biti vor fi 0.

 	Pentru ca v[1] depinde de noua valoare a lui v[0] iar v[3] de v[2], nu va putea fi facut un singur
 set de operatii. Intai trebuie calculate v[0] si v[2] (vect[0][0] si vect[0][2]) si apoi celelalte doua.
 Astfel, fac operatiile de adunare si xor de mai sus in mod vectorial si adun la vect[0] doar pozitiile
 pare, extrase prin spu_sel. Apoi recalculez a, b si c si fac operatiile adunand la vect[0] doar pozitiile
 impare. 
 	Se repeta procesul pentru fiecare element din vect.
 	Pentru decriptare, procesul este similar, cu mici diferente la rearanjarea octetilor.

 	DOUBLE BUFFERING
 	In mod normal, pe SPU se aloca un singur buffer in care se citesc datele de procesat. Pentru double
 bufferring se aloca doua buffere si se folosesc pe rand. Initial se face o cerere DMA pentru a aduce
 datele in primul buffer. Sa face o cerere si pentru al doilea si in timp ce datele sunt aduse, se proceseaza
 primul buffer. Apoi se face o cerere pentru noi date in acesta si se proceseaza celelalte date si tot asa.
 Astfel se ascunde latenta operatiilor de IO intrucat in timp ce se transmit date intr-unul din buffere, in
 celalalt se fac calcule, deci nu mai asteapta incheierea transferului.

	FISIERE MULTIPLE
	Pentru fisiere multiple se foloseste sincronizarea prin mailbox.
	PPU-ul se efectueaza urmatorii pasi:

	cat timp mai sunt fisiere de procesat:
		- citeste urmatoarea linie din fisierul multi si actualizeaza datele din structurile
	  		pentru SPU;
		- trimite 1 catre SPU-uri in inbound mailbox semn ca pot incepe procesarea
		- asteapta mesaj de la SPU-uri, semn ca au terminat operatiile
		- scrie rezultatul procesarilor in fisierul de output

	Comunicatia cu spu-urile se face intr-un singur thread, trimitand secvential mesaje prin mailbox
la fiecare SPU.

	Un SPU efectueaza urmatorii pasi:
	- ia adresa structurii corespunzatoare lui si transfera datele din ea;
	- proceseaza datele in functie de modurile alese
	- trimite un mesaj la PPU pentru ca s-a terminat procesarea
	- asteapta mesaj de la PPU: daca e 0 executia se termina
	                            altfel se reia de la primul pas

	Facand acest schimb de mesaje se asigura pentru PPU faptul ca se asteapta ca datele sa fie procesate
inainte de a fi scrise in fisier iar pentru SPU ca nu exista race condition in structura acestuia: intai
PPU-ul actualizeaza datele si apoi SPU le citeste.

	Pentru mai multe fisiere nu am mai verificat ca fiecare spu sa aiba minim 16 octeti de procesat, ramane
in grija programatorului sa nu testeze cu fisiere prea mici pentru numarul dat de spu-uri :).