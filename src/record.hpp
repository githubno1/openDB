/* Copyright 2013-2014 Salvatore Barone <salvator.barone@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __OPENDB_RECORD_HEADER__
#define __OPENDB_RECORD_HEADER__

#include <string>
#include <unordered_map>
#include <memory>
#include <fstream>
#include "column.hpp"
#include "exception.hpp"

namespace openDB{
/*
 */
class record {
public:
	/* Stato della tupla:
	 * -empty : una tupla viene creata emty quando bisogna, ad esempio, leggerla da file oppure da memoria. Costruire una tupla vuota non ha molto senso se non
	 * 			in questo contesto;
	 * - loaded : una tupla DEVE essere creata loaded nel caso in cui viene caricata dal database gestito;
	 * - inserting : una tupla viene creata inserting quando i dati che contiene devono essere inseriti nel database remoto;
	 * - updating : una tupla con stato updating è una tupla, già esistente nel database, i cui valori devono essere aggiornati
	 * - deleting: una tupla con stato deleting deve essere rimossa dal database;
	 */
	enum state {empty, loaded, inserting, updating, deleting};

	/* Costruttore
	 * Il costruttore senza argomenti costruisce una tupla vuota, utile nel caso in cui si debba leggere tuple da file;
	 * Il costruttore con argomenti consente di creare una tupla in modo corretto, inserendo opportunamente i dati. I paramentri sono:
	 * 	- valueMap : mappa il cui primo campo è il nome della colonna in cui inserire il valore contenuto nel secondo campo. Se non esiste nessuna colonna con il nome
	 * 				 specificato, viene generata una eccezione di tipo column_not_exists, derivata da access_exception. Se uno dei valori contenuti nel secondo campo
	 * 				 non fosse valido per il tipo di colonna al quale deve corrispondere, viene generata una eccezione derivata da data_exception. Vedi l'header
	 * 				 sqlType.hpp per i dettagli.
	 * 	- columnsMap : mappa delle colonne che compongono una tabella. Questo parametro viene utilizzato per la validazione dei valori contenuti in valueMap.
	 * 	- _state : rappresenta lo stato della tupla.
	 * Quando si crea un oggetto record con il costruttore con argomenti, possono essere generate le seguenti tipologie di eccezione:
	 * - key_empty : se ad una colonna chiave, o che compone la chiave, è associato un valore nullo.
	 * - column_not_exists : se una delle corrispondenze colonna-valore in valuesMap non è valida, cioè la colonna non esiste in columnsMap;
	 * - data_exception : viene generata una eccezione di tipo derivato da data_exception (vedi header 'exception.hpp') quando la corrispondenza colonna-valore non è
	 * 					  valida a causa di un errore dovuto ad un valore non compatibile con il tipo della colonna.
	 */
	record () throw () : __state(empty), __visible(false) {}
	record (std::unordered_map<std::string, std::string>& valuesMap, std::unordered_map<std::string, column>& columnsMap, enum state _state) throw (basic_exception&);

	/* La funzione update consente di marcare i valori di una tupla affinchè siano aggiornati correttamente. Prende i seguenti parametri:
	 * consente di creare una tupla in modo corretto, inserendo opportunamente i dati. I paramentri sono:
	 * 	- valueMap : mappa il cui primo campo è il nome della colonna in cui inserire il valore contenuto nel secondo campo. Se non esiste nessuna colonna con il nome
	 * 				 specificato, viene generata una eccezione di tipo column_not_exists, derivata da access_exception. Se uno dei valori contenuti nel secondo campo
	 * 				 non fosse valido per il tipo di colonna al quale deve corrispondere, viene generata una eccezione derivata da data_exception. Vedi l'header
	 * 				 sqlType.hpp per i dettagli.
	 * 	- columnsMap : mappa delle colonne che compongono una tabella. Questo parametro viene utilizzato per la validazione dei valori contenuti in valueMap.
	 * 	Quando si aggiorna un oggetto record possono essere generate le seguenti tipologie di eccezione:
	 * - column_not_exists : se una delle corrispondenze colonna-valore in valuesMap non è valida, cioè la colonna non esiste in columnsMap;
	 * - data_exception : viene generata una eccezione di tipo derivato da data_exception (vedi header 'exception.hpp') quando la corrispondenza colonna-valore non è
	 * 					  valida a causa di un errore dovuto ad un valore non compatibile con il tipo della colonna.
	 */
	void update (std::unordered_map<std::string, std::string>& valueMap, std::unordered_map<std::string, column>& columnsMap) throw (basic_exception&);

	/* La funzione cancel marca una tupla affinchè sia rimossa dal database remoto all'atto del commit. */
	void cancel () throw ()
		{__state = deleting; __visible = false;}

	/* restituisce lo stato in cui si trova la tupla */
	enum state state () const throw ()
			{return __state;}

	/* restituisce true se la tupla è visibile */
	bool visible () const throw ()
			{return __visible;}

	/* La funzione current restituisce un oggetto unordered_map il cui primo campo contiene il nome di una colonna mentre il secondo campo contiene il corrispettivo
	 * valore memorizzato dalla tupla.
	 * La funzione old restituisce un oggetto unordered_map il cui primo campo contiene il nome di una colonna mentre il secondo campo contiene il corrispettivo
	 * valore precedentemente memorizzato dalla tupla.
	 */
	std::unique_ptr<std::unordered_map<std::string, std::string>> current() const throw ();
	std::unique_ptr<std::unordered_map<std::string, std::string>> old() const throw ();


	/* La funzione size restituisce il numero di byte necessari alla memorizzazione su file della tupla.
	 * La funzione write consente la scrittura della tupla su stream binario. Viene generata una eccezione di tipo storage_exception nel caso in cui la scrittura non
	 * fosse possibile.
	 * La funzione read consente la lettura della tupla su stream binario. Viene generata una eccezione di tipo storage_exception nel caso in cui la lettura non
	 * fosse possibile.
	 */
	std::streamoff size() const throw ();
	void write (std::fstream& stream) const	throw (storage_exception&);
	void read (std::fstream& stream) throw (storage_exception&);

private:
	enum state											__state;
	struct 	value {
		std::string current;
		std::string old;
		value (std::string c, std::string o) : current(c), old(o) {}
	};
	std::unordered_map<std::string, value>				__valueMap;
	bool												__visible;

	void validate_column_name(std::unordered_map<std::string, std::string>& valueMap, std::unordered_map<std::string, column>& columnsMap) const throw (column_not_exists&);
	void validate_columns_value(std::unordered_map<std::string, std::string>& valueMap, std::unordered_map<std::string, column>& columnsMap) throw (data_exception&);
	void build_value_map(std::unordered_map<std::string, std::string>& valueMap, std::unordered_map<std::string, column>& columnsMap) throw (empty_key&);
	void update_value_map(std::unordered_map<std::string, std::string>& valueMap, std::unordered_map<std::string, column>& columnsMap) throw ();

};	/*	end of record declaration	*/
}; 	/*	end of openDB namespace	*/
#endif /* TUPLE_HPP_ */
