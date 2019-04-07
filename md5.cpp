#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <mutex>
#include <algorithm>
#include <openssl/md5.h>

using namespace std;

// struktura przechowujaca hasla
struct Password{
	string haslo;
	string hash;
};

class MD5Breaker {
private:
	vector <Password> NotBroken;
	vector <Password> Broken;
	vector <string> Dictionary;
	
	bool Run;
	bool Enabled;
	
	int task = 0;
public:
	mutex display_mutex; //mutex do wyswietlania tekstu
	mutex task_mutex; //mutex do przydzielenia zadania watkom
	mutex passwords_mutex; //mutex do hasel
	
	void SetRun(bool value){
		Run = value;
	}
	
	void SetEnabled(bool value){
		Enabled = value;
	}
	
	bool IsRunning(){
		return Run;
	}
	
	bool IsEnabled(){
		return Enabled;
	}

	void LoadPasswords() {
		string _line;
		Password pass;
		string plik;
		
		display_mutex.lock();
		cout << "Podaj nazwe pliku ze slownikiem: " << endl;
		cin >> plik; //nazwa pliku
		display_mutex.unlock();
		
		ifstream _file(plik);
		if (_file.is_open()) {
			while (getline(_file, _line)) {
				pass.hash = _line;
				NotBroken.push_back(pass);
			}
		}
		display_mutex.lock();
		cout << "Wgrano hasla do zlamania" << endl;
		display_mutex.unlock();
	}
	
	void LoadDictionary() {
		string _line;
		string plik;
		
		display_mutex.lock();
		cout << "Podaj nazwe pliku z haslami do zlamania: " << endl;
		cin >> plik; //nazwa pliku
		display_mutex.unlock();
		
		ifstream _file(plik);
		if (_file.is_open()) {
			while (getline(_file, _line)) {
				Dictionary.push_back(_line);
			}
		}
		display_mutex.lock();
		cout << "Wgrano slownik" << endl;
		display_mutex.unlock();
	}

	int DictionaryCount(){
		return Dictionary.size();
	}
	
	string GetWord(int i){
		return Dictionary[i];
	}
	
	void PrintBroken() {
		size_t i = 0;
		display_mutex.lock();
		while (i<Broken.size()) {
			cout << "Haslo: " << Broken[i].haslo << " hash: " << Broken[i].hash << endl;
			i++;
		}
		display_mutex.unlock();
	}

	void PrintNotBroken() {
		size_t i = 0;
		display_mutex.lock();
		while (i<NotBroken.size()) {
			cout << "Hash: " << NotBroken[i].hash << endl;
			i++;
		}
		display_mutex.unlock();
	}
	
	int NotBrokenCount(){
		return NotBroken.size();
	}
	
	void PrintDictionary() {
		
		size_t i = 0;
		display_mutex.lock();
		while (i<Dictionary.size()) {
			cout << Dictionary[i] << endl;
			i++;
		}
		display_mutex.unlock();
	}
	
	void PrintMenu() {
		
		display_mutex.lock();
		cout << endl;
		cout << "Wybierz co chcesz zrobic: " << endl;
		cout << "1) Wczytaj slownik" << endl;
		cout << "2) Wyswietl slownik" << endl;
		cout << "3) Wczytaj liste hasel" << endl;
		cout << "4) Wyswietl liste hasel" << endl;
		cout << "5) Uruchom prace watkow" << endl;
		cout << "6) Zatrzymaj prace watkow" << endl << endl;
		cout << "9) Zakoncz" << endl;
		display_mutex.unlock();
	}
	
	int GetTask(){
		return task++;
	}
	
	bool CompareHash(string h, string w){
		for(int i=0; i<NotBroken.size(); i++){
			if(h==NotBroken[i].hash){
				Password temp;
				temp.hash = h;
				temp.haslo = w;
				
				passwords_mutex.lock();
				Broken.push_back(temp);
				NotBroken.erase(NotBroken.begin()+i);
				passwords_mutex.unlock();
				
				return true;
			}
		}
		return false;
	}
	
	string toLower(string tekst){
		transform(tekst.begin(), tekst.end(), tekst.begin(), ::tolower);
		return tekst;
	}
	
	string firstUpper(string tekst){
		transform(tekst.begin(), tekst.begin()+1, tekst.begin(), ::toupper);
		return tekst;
	}
	
	string toUpper(string tekst){
		transform(tekst.begin(), tekst.end(), tekst.begin(), ::toupper);
		return tekst;
	}
	
	string MyMD5(string str){
		unsigned char digest[MD5_DIGEST_LENGTH];
		const char *cstr = str.c_str();
		MD5((unsigned char*)cstr, strlen(cstr), (unsigned char*)&digest);
		char mdString[33];
		
		for(int i = 0; i < 16; i++)
			sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
		
		string out = string(mdString);
		return out;
	}
	
};

void Crack(MD5Breaker& breaker) {
	thread::id tid = this_thread::get_id(); //id watku
	int task; //zadanie watku
	int i=0;
	int j=0;
	string word;
	string hash;
	
	breaker.task_mutex.lock();
	task = breaker.GetTask();
	breaker.task_mutex.unlock();
	
	breaker.display_mutex.lock();
	cout << "Uruchomiono watek " << tid << endl;
	breaker.display_mutex.unlock();
	
	while (breaker.IsEnabled()) {
		while (breaker.IsRunning()) {
			if(breaker.NotBrokenCount()==0){ //jesli znalezlismy wszystkie hashe to konczymy prace watkow
				breaker.SetRun(false);
			}
			
			// producenci jednowyrazowi
			if(task==0) word = breaker.toLower(breaker.GetWord(i));
			if(task==1) word = breaker.firstUpper(breaker.GetWord(i));
			if(task==2) word = breaker.toUpper(breaker.GetWord(i));
			
			// producenci dwuwyrazowi
			if(i>=1){
				if(task==3) word = breaker.toLower(breaker.GetWord(i-1) + breaker.GetWord(i));
				if(task==4) word = breaker.firstUpper(breaker.GetWord(i-1) + breaker.GetWord(i));
				if(task==5) word = breaker.toUpper(breaker.GetWord(i-1) + breaker.GetWord(i));
			}
			
			hash = breaker.MyMD5(word);
			
			if(breaker.CompareHash(hash, word)){
				breaker.display_mutex.lock();
				cout << "Watek " << tid << ":" << endl;
				cout << "Hash: " << hash << endl;
				cout << "Haslo: " << word << endl << endl;
				breaker.display_mutex.unlock();
			}
			
			i++;
			if(i>=breaker.DictionaryCount()){
				i=0;
				j++;
			}
		}
		sleep(5); //watek wlaczony, ale czeka na start pracy
	}
	
	breaker.display_mutex.lock();
	cout << "Zakonczono prace watku " << tid << endl;
	breaker.display_mutex.unlock();
}



int main()
{
	int cmd;
	MD5Breaker breaker;
	breaker.SetRun(false);
	breaker.SetEnabled(true);


	// uruchomienie wstrzymanych watkow
	thread t0(Crack, ref(breaker));
	thread t1(Crack, ref(breaker));
	thread t2(Crack, ref(breaker));
	thread t3(Crack, ref(breaker));
	thread t4(Crack, ref(breaker));
	thread t5(Crack, ref(breaker));

	// glowna petla programu
	do {
		breaker.PrintMenu(); // drukuje menu
		cin >> cmd; // wybor uzytkownika

		switch (cmd) {
		case 1: {
			breaker.LoadDictionary();
		}
				break;
		case 2: {
			breaker.PrintDictionary();
		}
				break;
		case 3: {
			breaker.LoadPasswords();
		}
				break;
		case 4: {
			breaker.PrintNotBroken();
		}
				break;
		case 5: {
			breaker.SetRun(true);
		}
			break;
		case 6: {
			breaker.SetRun(false);
		}
			break;
		default:{
			// nic nie robimy
		}
			break;
		}


	} while (cmd!=9); // komenda wyjscia

	// zatrzymanie i wylaczenie watkow
	breaker.SetRun(false);
	breaker.SetEnabled(false);

	// czekanie, az watki zakoncza swoja prace
	if (t0.joinable()) t0.join();
	if (t1.joinable()) t1.join();
	if (t2.joinable()) t2.join();
	if (t3.joinable()) t3.join();
	if (t4.joinable()) t4.join();
	if (t5.joinable()) t5.join();

    return 0;
}

