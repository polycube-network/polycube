Polyube metrics with Prometheus
=================================


Brief architecture
------------------





Prometheus-cpp
---------------
 The library chosen to create metrics was `Prometheus-cpp <https://github.com/jupp0r/prometheus-cpp.git>`_ which was added as a submodule to Polycube, modifying the gitsubmodules file and the CMakeLists.txt in polycube/src/polycubed/src (a comment for the future has also been added in the file scripts/pre-requirements.sh)



DID
----------
-[X] Estensioni aggiunte in polycube-base.yang
-[X] Metriche (counter, gauge) aggiunte nello yang di alcuni servizi
-[X] Lettura delle metriche (counter) nel web server, un servizio alla volta
-[X] Modifica prototipi in yangNodes.tcpp (è stato aggiunto il parametro ServiceMetadata md) per permettere di salvare
le metriche lette nello yang come metadata definiti in management_interface e a cui si può accedere usando una funzione
definita in service_controller che può essere a sua volta chiamata da un elemento di tipo core


TODO
-----------
-[] Aggiungere estensioni in augment, container
-[] Creare un ParseAugment
-[X] Nel rest server aggiungere la creazione di metriche (counter, gauge) in una funzione che viene chiamata una sola volta all'avvio
-[] Nel rest server
1) mappa <nome servizio, struct metrica> dove la struct metrica { vettore di reference TipoMetricaFamily, vettore di reference TipoMetrica }
2) loop { per ogni elemento della mappa si chiama la get_InfoMetrics, loop { per ogni metrica letta si creano i vettori } }
3)
-[] usare jsonpath per leggere il valore di una metrica dal json di un servizio a run-time
-[] aggiungere la libreria jsonpath (per ora in submodule)
-[] spostare le librerie prometheus-cpp e jsonpath in external libraries
-[] commentare yangnodes.tcpp

