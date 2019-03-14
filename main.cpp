#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <typeinfo>
#include <sstream>
#include <map>
#include <fstream>

using namespace std;

class CSVLine
{
private:
    map<string, string> collection; //klic je nazev sloupce, hodnota je hodnota bunky
public:
    string Get(string name) {
        return collection[name]; //vrati hodnotu bunky pro sloupec
    }
    void Add(string name) {
        collection[name] = ""; //nastavi prazdnou hodnotu bunky (uzitecne pro serializaci nazvu sloupcu)
    }
    void Add(string name, string value) {
        collection[name] = value; //ulozi hodnotu bunky pro dany sloupec
    }
    string GetClassID(){
        return collection["ClassID"]; //vrati unikatni identifikator tridy objektu
    }
    vector<string> GetColumns() {
        vector<string> columns;

        map<string, string>::iterator it;

        for ( it = collection.begin(); it != collection.end(); it++ )
        {
            columns.push_back(it->first); //pro sloupec ulozi jeho nazev a vrati kolekci
        }

        return columns;
    }
    string SerializeHeader() { //vrati nazev sloupcu jako retezec ve formatu CSV radku
        stringstream stream;

        map<string, string>::iterator it;

        for ( it = collection.begin(); it != collection.end(); it++ )
        {
            stream << "\"" << it->first << "\";";
        }

        return stream.str();
    }
    string SerializeBody(vector<string> columns) { //vrati obsah bunek jako retezec ve formatu CSV radku
        stringstream stream;

        vector<string>::iterator it;

        for( it = columns.begin(); it != columns.end(); ++it) {
            stream << "\"" << collection[*it] << "\";";
        }

        return stream.str();
    }
    vector<string> DeserializeHeader(string line) { //vrati nazvy sloupcu v kolekci z retezce ve formatu CSV radku
        vector<string> columns;

        stringstream stream(line);

        string item;

        while(getline(stream, item, ';')) { //strednik jako oddelovac natvrdo, ale neni takovy problem upravit
            item.erase(remove(item.begin(), item.end(), '"'), item.end());

            columns.push_back(item);
        }

        return columns;
    }
    void DeserializeBody(vector<string> columns, string line){ //ulozi obsah bunek z retezce ve formatu CSV radku
        stringstream stream(line);

        string item;

        int index = 0;

        while(getline(stream, item, ';')) {
            item.erase(remove(item.begin(), item.end(), '"'), item.end());

            collection[columns[index]] = item;

            index++;
        }
    }
};

class CSVObject //zakladni abstraktni trida kterou musi dedit vsechny objekty persistovane v CSV souboru
{
public:
    virtual string GetClassId() = 0; //abstraktni metoda pro zisakni jedinecneho identifikatoru tridy
    virtual void Serialize(CSVLine* line) { //virtualni metoda pro serializaci vsech properties objektu
        line->Add("ClassID", GetClassId());
    }
    virtual void Deserialize(CSVLine* line) {} //virtualni metoda pro deserializaci vsech properties objektu
    virtual void Print() { //vytisk vsech informaci o objektu do konzole
        cout << endl << "-----" << GetClassId() << "-----" << endl;
    }
};

CSVObject* GetObject(string classID);

class CSVStore //trida pro nakladni s persistovanymi objekty v CSV
{
private:
    map<string, vector<CSVObject*> > collection; //klic je identifikator tridy, hodnota je kolekce vsech instanci dane tridy
public:
    void Add(CSVObject* object) {
        collection[object->GetClassId()].push_back(object); //prida objekt do storu
    }
    void Serialize(char* fileName){ //serializace vsech objektu do CSV souboru
        ofstream fileStream(fileName);

        vector<CSVObject*> all;

        CSVLine* headerLine = new CSVLine();

        map<string, vector<CSVObject*> >::iterator it;

        //Do jednoho CSVLine serializujeme od kazde tridy jeden objekt.
        //Tim zarucime dostupnost vsech moznych nazvu sloupcu.
        for ( it = collection.begin(); it != collection.end(); it++ )
        {
            CSVObject* object = it->second[0];
            object->Serialize(headerLine);

            all.insert(all.end(), it->second.begin(), it->second.end());
        }

        vector<string> columns = headerLine->GetColumns(); //ziskame kolekci nazvu sloupcu

        fileStream << headerLine->SerializeHeader() << endl; //serializovane je zapisme do souboru

        vector<CSVObject*>::iterator it2;

        //Nyni serializujeme jednotlive instance (objekty)
        //V tuhle chvili uz je nutne mit seznam vsech moznych sloupcu
        for ( it2 = all.begin(); it2 != all.end(); it2++ )
        {
            CSVLine* line = new CSVLine();

            CSVObject* object = *it2;

            object->Serialize(line);

            fileStream << line->SerializeBody(columns) << endl;
        }

        fileStream.close();
    }
    vector<CSVObject*> Deserialize(char* fileName){ //deserializace vsech objektu z CSV souboru
        vector<CSVObject*> result;

        ifstream fileStream(fileName);

        string text;

        getline(fileStream, text);

        CSVLine* headerLine = new CSVLine();

        vector<string> columns = headerLine->DeserializeHeader(text); //ziskame nazvy vsech sloupcu

        //Kazdy objekt deserializujeme zase s pomoci kolekce nazvu sloupcu
        while (getline(fileStream, text))
        {
            CSVLine* line = new CSVLine();

            line->DeserializeBody(columns, text);

            string classID = line->GetClassID(); //zisakme identifikator tridy

            CSVObject* object = GetObject(classID); //zavolame funkci pro ziskani instance teto tridy

            object->Deserialize(line); //nyni muzeme zavolat deserializaci, ta uz bude probihat v metode daneho objektu

            result.push_back(object);

            Add(object); //pridame si objekt do kolekce
        }

        return result;
    }
};

class Employee : public CSVObject
{
public:
    string FirstName;
    string LastName;
    string Company;
    string Address;
    string Email;

    string GetClassId() override {
        return "Employee";
    }

    void Serialize(CSVLine* line) override { //provadime override virtualni metody pro serializaci
        CSVObject::Serialize(line); //nesmime zapomenout volat bazove metody
        line->Add("First name", FirstName);
        line->Add("Last name", LastName);
        line->Add("Company", Company);
        line->Add("Address", Address);
        line->Add("Email", Email);
    }

    void Deserialize(CSVLine* line) override {
        CSVObject::Deserialize(line);
        FirstName = line->Get("First name");
        LastName = line->Get("Last name");
        Company = line->Get("Company");
        Address = line->Get("Address");
        Email = line->Get("Email");
    }

    void Print() override {
        CSVObject::Print();
        cout << "First name: " << FirstName << endl;
        cout << "Last name: " << LastName << endl;
        cout << "Company: " << Company << endl;
        cout << "Address: " << Address << endl;
        cout << "Email: " << Email << endl;
    }
};

class Teacher : public Employee
{
public:
    string Subject;

    string GetClassId() override {
        return "Teacher";
    }

    void Serialize(CSVLine* line) override {
        Employee::Serialize(line);
        line->Add("Subject", Subject);
    }

    void Deserialize(CSVLine* line) override {
        Employee::Deserialize(line);
        Subject = line->Get("Subject");
    }

    void Print() override {
        Employee::Print();
        cout << "Subject: " << Subject << endl;
    }
};

class Programmer : public Employee
{
public:
    string Language;

    string GetClassId() override {
        return "Programmer";
    }

    void Serialize(CSVLine* line) override {
        Employee::Serialize(line);
        line->Add("Language", Language);
    }

    void Deserialize(CSVLine* line) override {
        Employee::Deserialize(line);
        Language = line->Get("Language");
    }

    void Print() override {
        Employee::Print();
        cout << "Language: " << Language << endl;
    }
};

class FilmCharacterEmployee : public Employee
{
public:
    string Role;

    string GetClassId() override {
        return "FilmCharacterEmployee";
    }

    void Serialize(CSVLine* line) override {
        Employee::Serialize(line);
        line->Add("Role", Role);
    }

    void Deserialize(CSVLine* line) override {
        Employee::Deserialize(line);
        Role = line->Get("Role");
    }

    void Print() override {
        Employee::Print();
        cout << "Role: " << Role << endl;
    }
};

CSVObject* GetObject(string classID){ //nezapomenout pridat vsechny persistovane tridy, jinak nebude fungovat
    if(classID == "Employee") return new Employee();
    if(classID == "Teacher") return new Teacher();
    if(classID == "Programmer") return new Programmer();
    if(classID == "FilmCharacterEmployee") return new FilmCharacterEmployee();
}

int main()
{
    char* file = "C:/Users/Adam/Desktop/testcsv.csv"; //zde cesta k souboru TODO: z argumentu

    cout << "Creating my objects...";

    Teacher *pavel = new Teacher();
    pavel->FirstName = "Pavel";
    pavel->LastName = "Novak";
    pavel->Company = "Czech technical university";
    pavel->Address = "Praha";
    pavel->Email = "pavel.novak@seznam.cz";
    pavel->Subject = "Computer science";

    Programmer *karel = new Programmer();
    karel->FirstName = "Karel";
    karel->LastName = "Novak";
    karel->Company = "Red Hat";
    karel->Address = "Brno";
    karel->Email = "karel.novak@seznam.cz";
    karel->Language = "C++";

    Teacher *petr = new Teacher();
    petr->FirstName = "Petr";
    petr->LastName = "Novak";
    petr->Company = "University of Technology";
    petr->Address = "Brno";
    petr->Email = "petr.novak@seznam.cz";
    petr->Subject = "Computer science";

    cout << "done" << endl;

    cout << "Saving objects to CSV file...";

    CSVStore *store = new CSVStore();
    store->Add(pavel);
    store->Add(karel);
    store->Add(petr);
    store->Serialize(file);

    cout << "done" << endl;

    cout << "Loading objects from CSV file...";

    store = new CSVStore();
    vector<CSVObject*> objects = store->Deserialize(file);

    cout << "done" << endl;

    cout << "Printing objects information..." << endl;

    vector<CSVObject*>::iterator it;

    for ( it = objects.begin(); it != objects.end(); it++ )
    {
        CSVObject* object = *it;

        object->Print();
    }

    cout << endl << "Adding new object...";

    FilmCharacterEmployee *james = new FilmCharacterEmployee();
    james->FirstName = "James";
    james->LastName = "Bond";
    james->Company = "MI6";
    james->Address = "Top secret";
    james->Email = "james.bond@mi6.co.uk";
    james->Role = "Agent";
    store->Add(james);

    cout << "done" << endl;

    cout << "Saving with new objects to CSV file...";

    store->Serialize(file);

    cout << "done" << endl;

    cout << "Loading objects from CSV file again...";

    store = new CSVStore();
    objects = store->Deserialize(file);

    cout << "done" << endl;

    cout << "Printing objects information again..." << endl;

    for ( it = objects.begin(); it != objects.end(); it++ )
    {
        CSVObject* object = *it;

        object->Print();
    }

    return 0;
}
