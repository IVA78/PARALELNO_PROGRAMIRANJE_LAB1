# Paralelno programiranje - Prva domaća zadaća

## Zadatak: Simulacija raspodijeljenog problema n filozofa uporabom MPI-a

### Opis problema
Ova simulacija temelji se na klasičnom problemu "n filozofa" prilagođenom za raspodijeljeno okruženje.

Model obuhvaća **n filozofa** koji sjede za okruglim stolom i dijele **n vilica**. Svaki filozof treba **dvije vilice** kako bi mogao jesti, pri čemu svaka vilica može biti korištena samo od strane dva susjedna filozofa.

Za razliku od uobičajene inačica problema s zajedničkim spremnikom, filozofi su implementirani kao **nezavisni procesi** koji međusobno komuniciraju **isključivo razmjenom poruka** (MPI).

Vilice **nisu na stolu**, već su uvijek u posjedu određenog filozofa. Na početku, svaka vilica pripada filozofu s nižem indeksom. To znači da filozof s **indeksom 0** početno ima dvije vilice, dok filozof s indeksom **n-1** nema nijednu.

Program se mora moći pokrenuti s proizvoljnim brojem procesa **(n > 1)**. Ispis mora biti prilagođen tako da svaki proces (filozof) ispisuje promjene stanja, pri čemu se koristi **uvlačenje teksta** proporcionalno indeksu procesa.

### Pravila ponašanja filozofa
1. Filozof jede ako ima obje vilice (neovisno o čistoći vilica).
2. Nakon jela, obje vilice postaju **prljave**.
3. Ako filozof želi jesti, šalje **zahtjev** za vilicu koja nije kod njega i čeka odgovor.
4. Ako filozof ne jede, a postoji zahtjev za **prljavu** vilicu koju posjeduje, on je čisti i šalje susjedu.
5. Svaki filozof pamti zahtjeve za vilicama koje je primio od susjeda.
6. Dok filozof **misli**, udovoljava svakom zahtjevu za vilicom koju posjeduje.

Filozof ne može udovoljiti zahtjevu za **čistu** vilicu. Ona se može predati tek nakon što postane prljava (nakon jela).

### Implementacija algoritma
Filozofi se ponašaju prema sljedećem ciklusu:
```c
Proces(i) {
    misli (slucajan broj sekundi);
    i 'istovremeno' odgovaraj na zahtjeve!
    dok (nemam obje vilice) {
        posalji zahtjev za vilicom;
        ponavljaj {
            cekaj poruku (bilo koju!);
            ako je poruka odgovor na zahtjev
                azuriraj vilice;
            inace ako je poruka zahtjev
                obradi zahtjev (odobri ili zabiljezi);
        } dok ne dobijes trazenu vilicu;
    }
    jedi;
    odgovori na postojeće zahtjeve;
}
```
### Ispis stanja filozofa
Svaki filozof ispisuje svoje stanje tijekom izvođenja:
- `mislim`
- `tražim vilicu (indeks)`
- `dobio vilicu`
- `drugi traže moju vilicu`
- `jedem`

### Asinkrono odgovaranje na zahtjeve
Filozof mora povremeno provjeravati postoji li novi zahtjev upućen njemu.
To se može postići **neblokirajućim** pozivom **MPI_Iprobe**, koji provjerava postoji li nova poruka.

Kada `MPI_Iprobe` detektira poruku, ona se zatim prima pomoću `MPI_Recv`. U ovom zadatku, interval provjeravanja može biti postavljen na **1 sekundu**.

### Literatura
Algoritam koji treba implementirati temelji se na radu:
**K.M. Chandy, J. Misra: "The drinking philosophers problem"** ([link](https://www.fer.unizg.hr/_download/repository/Philosophers.pdf), poglavlje 4).

