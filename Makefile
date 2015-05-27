################################
#
#     Makefile WATOR (progetto lso 2015)
#     (fram 1)(fram 2)
################################

# ***** DA COMPLETARE ******  con i file da consegnare *.c e *.h     
# primo frammento 
FILE_DA_CONSEGNARE1=wator.c wator.h

# secondo frammento 
FILE_DA_CONSEGNARE2=utils.c utils.h wator_process.c wator_process.h visualizer.c visualizer.h wator.c wator.h

# Compilatore
CC= gcc
# Flag di compilazione
CFLAGS = -Wall -pedantic -g

# Librerie 
# Directory in cui si trovano le librerie
LIBDIR = ../lib
# Opzione di linking
LIBS = -L $(LIBDIR)

# Nome libreria progetto
LIBNAME1 = libWator.a
LIBNAME2 = libUtils.a

# Oggetti libreria $(LIBNAME1)
# DA COMPLETARE (se si usano altri file sorgente)
objects1 = wator.o
objects2 = utils.o





# Nome eseguibili primo frammento
EXE1=shark1
EXE2=shark2
EXE3=shark3

.PHONY: clean cleanall lib libUtils test11 test12 consegna1 docu


# creazione libreria 
lib:  $(objects1)
	-rm  -f $(LIBNAME1)
	-rm  -f $(LIBDIR)/$(LIBNAME1)
	ar -r $(LIBNAME1) $(objects1)
	cp $(LIBNAME1) $(LIBDIR)


###### Primo test 
shark1: test-one.o 
	$(CC) -o $@  $^ $(LIBS) -lWator

test-one.o: test-one.c wator.h 
	$(CC) $(CFLAGS) -c $<	 

###### Secondo test 
shark2: test-two.o 
	$(CC) -o $@  $^ $(LIBS) -lWator

test-two.o: test-two.c wator.h 
	$(CC) $(CFLAGS) -c $<	 

###### Terzo test 
shark3: test-three.o 
	$(CC) -o $@  $^ $(LIBS) -lWator

test-three.o: test-three.c wator.h 
	$(CC) $(CFLAGS) -c $<	 

#make rule per gli altri .o del primo frammento (***DA COMPLETARE***)


#### secondo frammento
# creazione libreria
lib2:  $(objects2)
	-rm  -f $(LIBNAME2)
	-rm  -f $(LIBDIR)/$(LIBNAME2)
	ar -r $(LIBNAME2) $(objects2)
	cp $(LIBNAME2) $(LIBDIR)

######### target visualizer e wator (da completare)
wator: wator_process.o
	$(CC) -o $@ $^ $(LIBS) -lWator -lUtils -pthread
	
visualizer: visualizer.o
	$(CC) -o $@ $^ $(LIBS) -lUtils -pthread

wator_process.o: wator_process.c wator_process.h $(LIBNAME1) $(LIBNAME2)
	$(CC) $(CFLAG) -c $<

visualizer.o: visualizer.c visualizer.h
	$(CC) $(CFLAG) -c $<

# make rule per gli altri .o del secondo/terzo frammento (***DA COMPLETARE***)
utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $<


########### NON MODIFICARE DA QUA IN POI ################
# genera la documentazione con doxygen
docu: ../doc/Doxyfile
	make -C ../doc/

#ripulisce  l'ambiente 
clean:
	find . -name "*.o" -print -exec rm {} \; 

# cancella i file temporanei e tutto quello che devo ricreare/copiare
cleanall: clean
	\rm -f wator visualizer 
	\rm -f wator_worker_?* visualizer_dump out.wator out*.check planet?.check __wator.log wator.check planet?.dat
	\rm -f *~
	\rm -f $(EXE1) $(EXE2) $(EXE3) $(LIBDIR)/$(LIBNAME1) $(LIBNAME1)

# target di test
FILE1=planet1.check
FILE2=planet2.check
FILE3=planet3.check
DATA1=planet1.dat
DATA2=planet2.dat
OUT1=out1.check
OUT2=out2.check
CONF1=wator.conf.1
CONF2=wator.conf.2
MTRACEFILE=./__wator.log
test11: 
	make clean
	make lib
	make $(EXE1)
	-rm -fr $(FILE1) $(FILE2)  $(OUT1) $(OUT2)
	cp DATA/$(OUT1) .
	cp DATA/$(OUT2) .
	cp DATA/$(CONF1) .
	chmod 644 $(OUT1) $(OUT2) $(CONF1)
	mv $(CONF1) wator.conf
	-rm -fr $(MTRACEFILE)
	MALLOC_TRACE=$(MTRACEFILE) ./$(EXE1)
	mtrace ./$(EXE1) $(MTRACEFILE)
	diff $(FILE1) $(OUT1)
	diff $(FILE2) $(OUT2)
	diff $(FILE3) $(OUT1)
	@echo "********** Test11 superato!"

DATA4=planet4.dat
CONF3=wator.conf.2
test12: 
	make clean
	make lib
	make $(EXE2)
	-rm -fr $(DATA4) $(DATA2) 
	cp DATA/$(DATA4) .
	cp DATA/$(DATA2) .
	cp DATA/$(CONF3) .
	chmod 644 $(DATA4) $(DATA2) $(CONF3)	
	mv $(CONF3) wator.conf
	-rm -fr $(MTRACEFILE)
	MALLOC_TRACE=$(MTRACEFILE) ./$(EXE2)
	mtrace ./$(EXE2) $(MTRACEFILE)
	@echo "********** Test12 superato!"

test13: 
	make clean
	make lib
	make $(EXE3)
	-rm -fr $(DATA1) $(DATA2) 
	cp DATA/$(DATA1) .
	cp DATA/$(DATA2) .
	cp DATA/$(CONF2) .
	chmod 644 $(DATA1) $(DATA2) $(CONF2)
	mv $(CONF2) wator.conf
	-rm -fr $(MTRACEFILE)
	MALLOC_TRACE=$(MTRACEFILE) ./$(EXE3)
	mtrace ./$(EXE3) $(MTRACEFILE)
	@echo "********** Test13 superato!"

# test secondo frammento
# test script
OUTSCRIPT=out.wator
test21:
	cp DATA/$(DATA1) .
	cp DATA/$(DATA2) .
	cp DATA/$(CONF1) .
	cp DATA/$(OUTSCRIPT).check .
	chmod 644 $(DATA1) $(DATA2) $(CONF1) $(OUTSCRIPT).check
	mv $(CONF1) wator.conf	
	./testscript  1> $(OUTSCRIPT)
	diff $(OUTSCRIPT) $(OUTSCRIPT).check
	@echo "********** Test21 superato!"

# test libreria di comunicazione


# test attivazione e checkpointing
test22:
	make clean
	make lib
	make wator
	make visualizer
	-rm -fr $(DATA1) $(DATA2) 
	cp DATA/$(DATA1) .
	cp DATA/$(DATA2) .
	cp DATA/$(CONF2) .
	chmod 644 $(DATA1) $(DATA2) $(CONF2)
	rm -fr ./tmp
	mkdir ./tmp
	./testseq
	@echo "********** Test22 superato!"

# test comunicazione wator visualizzatore 
test23:
	make clean
	make lib
	make wator
	make visualizer
	-rm -fr $(DATA1) $(DATA2) 
	cp DATA/$(DATA1) .
	cp DATA/$(DATA2) .
	cp DATA/$(CONF2) .
	chmod 644 $(DATA1) $(DATA2) $(CONF2)
	rm -fr ./tmp
	mkdir ./tmp
	./testcom
	@echo "********** Test23 superato!"


SUBJECT1A="lso15: consegna primo frammento - corso A"
SUBJECT1B="lso15: consegna primo frammento - corso B"
ADDRESS="lso.di@listgateway.unipi.it"
# target di consegna del primo frammento
# effettua i test e prepara il tar per la consegna
consegna1:
	make clean
	make test11
	make test12
	make test13
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f1.tar ./gruppo.txt ./Makefile $(FILE_DA_CONSEGNARE1) 
	@echo "*** PRIMO FRAMMENTO: TAR PRONTO $(USER)-f1.tar "
	@echo "inviare come allegato a lso.di@listgateway.unipi.it con subject:"
	@echo "$(SUBJECT1A) oppure"
	@echo "$(SUBJECT1B)"




SUBJECT2A="lso15: consegna secondo frammento - corso A"
SUBJECT2B="lso15: consegna secondo frammento - corso B"
# target di consegna del secondo frammento
# effettua i test e prepara il tar per la consegna
consegna2:
	make clean
	make test21
	make test22
	make test23
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f2.tar ./gruppo.txt ./Makefile $(FILE_DA_CONSEGNARE2) 
	@echo "*** SECONDO FRAMMENTO: TAR PRONTO $(USER)-f2.tar "
	@echo "inviare come allegato a lso.di@listgateway.unipi.it con subject:"
	@echo "$(SUBJECT2A) oppure"
	@echo "$(SUBJECT2B)"

