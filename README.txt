Sisteme de Operare

Tema 1 - Loader de executabile ELF


Apelurile de sistem folosite pentru implementare sunt:

-> mmap() cu flag-ul MAP_ANON, pentru alocarea anonima a memoriei,
precum si initializarea acelei zone cu 0

-> signal() cu flag-ul SIG_DFL, pentru cazurile de acces invalid sau
nepermis la memorie, care schimba handler-ul meu de SIGSEGV cu cel
default al sistemului

-> raise(), folosit dupa signal(), pentru generarea semnalului imediat
dupa schimbarea in handler-ul default

-> lseek(), pentru a muta in fisierul ELF dat ca argument offset-ul de
la care va citi date in memoria mapata anonim

-> read(), pentru citirea efectiva in zona alocata de memorie a datelor
din fisier

-> mprotect(), pentru atribuirea permisiunilor mentionate in header-ul
segmentului paginii mapate


Logica handler-ului urmareste calculul adresei de la care trebuie mapata
fiecare pagina, in functie de segmentul in care se afla adresa de page fault.
In cazul in care adresa de page fault nu e in niciun segment, se schimba
handler-ul in cel default si se opreste executia programului. Dupa aceea,
pentru adrese valide, se face alocarea de memorie cu MAP_ANON si
MAP_FIXED_NOREPLACE de la adresa de baza a paginii calculata. Deoarece este
folosit MAP_FIXED_NOREPLACE, acesta verifica daca zona de memorie de la care
se incearca alocarea a mai fost deja mapata, caz in care este un acces nepermis
de memorie si returneaza eroare EEXIST in errno. Se verifica daca s-a produs
acces nepermis la memorie, iar acesta constituie alt caz in care se schimba
handler-ul in cel default. Se schimba cursorul in fisierul ce trebuie incarcat
la offset-ul din program header, iar apoi, pe cazuri, se copiaza memoria din
fisier in spatiul de adrese virtuale.

In cazul in care toata pagina se afla in [vaddr, vaddr + file_size], se
citeste din fisier la adresa de baza a paginii exact o pagina.

In cazul in care o parte din pagina se afla in [vaddr, vaddr + file_size],
iar cealalta parte se afla in [vaddr + file_size, vaddr + mem_size], se
citeste din fisier la adresa de baza a paginii doar numarul de bytes din
intervalul [vaddr, vaddr + file_size].

Nu tratez explicit cazurile legate de zona dintre vaddr + file_size si
vaddr + mem_size deoarece aceasta trebuie initializata cu 0, lucru care
se intampla la alocarea cu flag-ul MAP_ANON.

Ultimul lucru pe care il face handler-ul este sa puna permisiunile mentionate
in program header pe pagina mapata, deoarece la apelul lui mmap, am alocat cu
permisiuni de read-write, pentru a putea scrie peste zona de memorie datele din
fisier.