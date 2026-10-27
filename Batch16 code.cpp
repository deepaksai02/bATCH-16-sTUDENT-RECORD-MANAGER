// Student Record Manager - PBL: Object Oriented Programming through C++ (2501CS08)
// Layers: Presentation (console menu) -> Business Logic (managers) -> Data (file handling)
// Written in standard C++98 so it compiles in Dev-C++ without changing any compiler option.
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cctype>
#include <map>
#include <string>
#include <vector>
using namespace std;

// ---------------------------------------------------------------- Student
// Base class: encapsulates personal details (protected data, public accessors).
class Student {
protected:
    string rollNo, name, department, contact;
public:
    Student(string r, string n, string d, string c)
        : rollNo(r), name(n), department(d), contact(c) {}
    virtual ~Student() {}

    // Polymorphism: course-specific rules differ for UG and PG students.
    virtual string category() const = 0;
    virtual int passMark() const = 0;

    const string& getRoll() const { return rollNo; }
    const string& getName() const { return name; }
    const string& getDepartment() const { return department; }
    const string& getContact() const { return contact; }
    void setName(const string& n) { name = n; }
    void setDepartment(const string& d) { department = d; }
    void setContact(const string& c) { contact = c; }
};

// Inheritance: UGStudent and PGStudent derive from Student.
class UGStudent : public Student {
public:
    UGStudent(string r, string n, string d, string c) : Student(r, n, d, c) {}
    string category() const { return "UG"; }
    int passMark() const { return 40; }
};

class PGStudent : public Student {
public:
    PGStudent(string r, string n, string d, string c) : Student(r, n, d, c) {}
    string category() const { return "PG"; }
    int passMark() const { return 50; }
};

// ------------------------------------------------------------------ Marks
class Marks {
    map<string, int> subjects;               // subject name -> marks (out of 100)
public:
    typedef map<string, int>::const_iterator Iter;

    void setMark(const string& subject, int m) { subjects[subject] = m; }
    const map<string, int>& all() const { return subjects; }
    bool empty() const { return subjects.empty(); }
    double average() const {
        if (subjects.empty()) return 0;
        int total = 0;
        for (Iter it = subjects.begin(); it != subjects.end(); ++it)
            total += it->second;
        return (double)total / subjects.size();
    }
    static string gradeOf(int m) {
        if (m >= 90) return "O";
        if (m >= 80) return "A+";
        if (m >= 70) return "A";
        if (m >= 60) return "B";
        if (m >= 50) return "C";
        if (m >= 40) return "D";
        return "F";
    }
    string grade() const { return gradeOf((int)(average() + 0.5)); }
};

// ------------------------------------------------------------- Attendance
class Attendance {
    int totalDays, presentDays;
public:
    Attendance() : totalDays(0), presentDays(0) {}
    void mark(bool present) { totalDays++; if (present) presentDays++; }
    void set(int total, int present) { totalDays = total; presentDays = present; }
    int total() const { return totalDays; }
    int present() const { return presentDays; }
    int absent() const { return totalDays - presentDays; }
    double percentage() const { return totalDays ? 100.0 * presentDays / totalDays : 0; }
};

// ----------------------------------------------------------------- Record
// Links a student to their marks and attendance data. Owns the Student object.
class Record {
    Record(const Record&);                   // not copyable (owns a pointer)
    Record& operator=(const Record&);
public:
    Student* student;
    Marks marks;
    Attendance attendance;
    Record(Student* s) : student(s) {}
    ~Record() { delete student; }
};

// ---------------------------------------------------------- Data Layer
vector<string> split(const string& line, char sep) {
    vector<string> parts;
    string part;
    stringstream ss(line);
    while (getline(ss, part, sep)) parts.push_back(part);
    return parts;
}

class FileHandler {
    string path;
public:
    FileHandler(string p) : path(p) {}

    // format: type|roll|name|department|contact|total|present|subject:mark;subject:mark;
    void save(const vector<Record*>& records) const {
        ofstream out(path.c_str());
        for (size_t i = 0; i < records.size(); i++) {
            const Record& r = *records[i];
            out << r.student->category() << '|' << r.student->getRoll() << '|'
                << r.student->getName() << '|' << r.student->getDepartment() << '|'
                << r.student->getContact() << '|' << r.attendance.total() << '|'
                << r.attendance.present() << '|';
            for (Marks::Iter m = r.marks.all().begin(); m != r.marks.all().end(); ++m)
                out << m->first << ':' << m->second << ';';
            out << '\n';
        }
    }

    void load(vector<Record*>& records) const {
        ifstream in(path.c_str());
        string line;
        while (getline(in, line)) {
            vector<string> f = split(line, '|');
            if (f.size() < 7) continue;
            Student* s;
            if (f[0] == "PG") s = new PGStudent(f[1], f[2], f[3], f[4]);
            else              s = new UGStudent(f[1], f[2], f[3], f[4]);
            Record* rec = new Record(s);
            rec->attendance.set(atoi(f[5].c_str()), atoi(f[6].c_str()));
            if (f.size() > 7) {
                vector<string> items = split(f[7], ';');
                for (size_t i = 0; i < items.size(); i++) {
                    size_t p = items[i].find(':');
                    if (p != string::npos)
                        rec->marks.setMark(items[i].substr(0, p), atoi(items[i].substr(p + 1).c_str()));
                }
            }
            records.push_back(rec);
        }
    }
};

// ------------------------------------------------- Business Logic Layer
string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

class StudentManager {
    vector<Record*> records;
    FileHandler& file;
    int nextId;
public:
    StudentManager(FileHandler& f) : file(f), nextId(1) {
        file.load(records);
        for (size_t i = 0; i < records.size(); i++) {       // continue after the highest ID
            int n = atoi(records[i]->student->getRoll().substr(5).c_str());
            if (n >= nextId) nextId = n + 1;
        }
    }
    ~StudentManager() {
        for (size_t i = 0; i < records.size(); i++) delete records[i];
    }
    vector<Record*>& all() { return records; }
    void save() { file.save(records); }

    // Registration Module: enrolment with automatic ID generation.
    string registerStudent(const string& type, const string& name,
                           const string& dept, const string& contact) {
        ostringstream id;
        id << "SRM26" << setw(3) << setfill('0') << nextId++;
        Student* s;
        if (type == "PG") s = new PGStudent(id.str(), name, dept, contact);
        else              s = new UGStudent(id.str(), name, dept, contact);
        records.push_back(new Record(s));
        save();
        return id.str();
    }

    Record* find(const string& roll) {
        for (size_t i = 0; i < records.size(); i++)
            if (records[i]->student->getRoll() == roll) return records[i];
        return NULL;
    }

    vector<Record*> searchByName(const string& key) {
        vector<Record*> result;
        string k = toLower(key);
        for (size_t i = 0; i < records.size(); i++)
            if (toLower(records[i]->student->getName()).find(k) != string::npos)
                result.push_back(records[i]);
        return result;
    }

    bool remove(const string& roll) {
        for (size_t i = 0; i < records.size(); i++)
            if (records[i]->student->getRoll() == roll) {
                delete records[i];
                records.erase(records.begin() + i);
                save();
                return true;
            }
        return false;
    }
};

class MarksManager {
    StudentManager& sm;
public:
    MarksManager(StudentManager& s) : sm(s) {}
    bool setMark(const string& roll, const string& subject, int m) {
        Record* r = sm.find(roll);
        if (!r || m < 0 || m > 100) return false;
        r->marks.setMark(subject, m);
        sm.save();
        return true;
    }
};

class AttendanceManager {
    StudentManager& sm;
public:
    AttendanceManager(StudentManager& s) : sm(s) {}
    bool mark(const string& roll, bool present) {
        Record* r = sm.find(roll);
        if (!r) return false;
        r->attendance.mark(present);
        sm.save();
        return true;
    }
};

class ReportManager {
    StudentManager& sm;
public:
    ReportManager(StudentManager& s) : sm(s) {}

    void reportCard(const string& roll) {
        Record* r = sm.find(roll);
        if (!r) { cout << "  Student not found.\n"; return; }
        const Student& s = *r->student;
        cout << "\n  ================ REPORT CARD ================\n";
        cout << "  Roll No    : " << s.getRoll() << "\n  Name       : " << s.getName()
             << "\n  Department : " << s.getDepartment() << "   (" << s.category() << ")\n";
        cout << "  ---------------------------------------------\n";
        cout << "  " << left << setw(24) << "Subject" << setw(8) << "Marks" << "Grade\n";
        cout << "  ---------------------------------------------\n";
        for (Marks::Iter m = r->marks.all().begin(); m != r->marks.all().end(); ++m)
            cout << "  " << left << setw(24) << m->first << setw(8) << m->second
                 << Marks::gradeOf(m->second) << "\n";
        cout << "  ---------------------------------------------\n";
        cout << fixed << setprecision(2);
        cout << "  Average    : " << r->marks.average() << "   Overall Grade: " << r->marks.grade() << "\n";
        cout << "  Attendance : " << r->attendance.present() << "/" << r->attendance.total()
             << " days (" << r->attendance.percentage() << "%)\n";
        bool pass = !r->marks.empty() && r->marks.average() >= s.passMark();
        cout << "  Result     : " << (pass ? "PASS" : "FAIL") << "  (" << s.category()
             << " pass mark: " << s.passMark() << ")\n";
        cout << "  =============================================\n";
    }

    void summary() {
        cout << "\n  ------------- ACADEMIC SUMMARY -------------\n";
        cout << "  " << left << setw(11) << "Roll No" << setw(18) << "Name" << setw(5) << "Type"
             << setw(9) << "Average" << setw(7) << "Grade" << "Attend.\n";
        cout << fixed << setprecision(1);
        for (size_t i = 0; i < sm.all().size(); i++) {
            Record& r = *sm.all()[i];
            cout << "  " << left << setw(11) << r.student->getRoll() << setw(18) << r.student->getName()
                 << setw(5) << r.student->category() << setw(9) << r.marks.average()
                 << setw(7) << r.marks.grade() << r.attendance.percentage() << "%\n";
        }
        cout << "  Total students: " << sm.all().size() << "\n";
    }
};

// ------------------------------------------------- Presentation Layer
string ask(const string& prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

void listStudents(StudentManager& sm) {
    cout << "\n  " << left << setw(11) << "Roll No" << setw(18) << "Name" << setw(7) << "Type"
         << setw(8) << "Dept" << "Contact\n";
    cout << "  ---------------------------------------------------------\n";
    for (size_t i = 0; i < sm.all().size(); i++) {
        Record& r = *sm.all()[i];
        cout << "  " << left << setw(11) << r.student->getRoll() << setw(18) << r.student->getName()
             << setw(7) << r.student->category() << setw(8) << r.student->getDepartment()
             << r.student->getContact() << "\n";
    }
    cout << "  Total records: " << sm.all().size() << "\n";
}

void adminMenu(StudentManager& sm) {
    cout << "\n  --- Admin Module ---\n  1. Register student\n  2. List all students\n"
            "  3. Search by name\n  4. Update student\n  5. Delete student\n  0. Back\n";
    string c = ask("  Choice: ");
    if (c == "1") {
        string t = ask("  Type (UG/PG): ");
        string n = ask("  Name: ");
        string d = ask("  Department: ");
        string p = ask("  Contact: ");
        string id = sm.registerStudent(t == "PG" ? "PG" : "UG", n, d, p);
        cout << "  Student registered. Generated ID: " << id << "\n";
    } else if (c == "2") {
        listStudents(sm);
    } else if (c == "3") {
        vector<Record*> res = sm.searchByName(ask("  Enter name to search: "));
        if (res.empty()) cout << "  No matching student found.\n";
        for (size_t i = 0; i < res.size(); i++)
            cout << "  " << res[i]->student->getRoll() << "  " << res[i]->student->getName() << "  ("
                 << res[i]->student->category() << ", " << res[i]->student->getDepartment() << ")\n";
    } else if (c == "4") {
        Record* r = sm.find(ask("  Roll No: "));
        if (!r) { cout << "  Student not found.\n"; return; }
        string n = ask("  New contact: ");
        r->student->setContact(n);
        sm.save();
        cout << "  Record updated.\n";
    } else if (c == "5") {
        cout << (sm.remove(ask("  Roll No to delete: ")) ? "  Record deleted.\n" : "  Student not found.\n");
    }
}

int main() {
    FileHandler file("records.txt");
    StudentManager sm(file);
    MarksManager mm(sm);
    AttendanceManager am(sm);
    ReportManager rm(sm);

    while (true) {
        cout << "\n========================================\n"
                "        STUDENT RECORD MANAGER\n"
                "========================================\n"
                "  1. Admin (add / update / delete / search)\n"
                "  2. Marks Management\n"
                "  3. Attendance\n"
                "  4. Report Card\n"
                "  5. Academic Summary\n"
                "  0. Exit\n";
        string c = ask("  Enter choice: ");
        if (c == "1") adminMenu(sm);
        else if (c == "2") {
            string roll = ask("  Roll No: ");
            string sub = ask("  Subject: ");
            int m = atoi(ask("  Marks (0-100): ").c_str());
            cout << (mm.setMark(roll, sub, m) ? "  Marks saved.\n" : "  Invalid roll number or marks.\n");
        } else if (c == "3") {
            string roll = ask("  Roll No: ");
            string st = ask("  Present today? (y/n): ");
            cout << (am.mark(roll, st == "y" || st == "Y") ? "  Attendance recorded.\n" : "  Student not found.\n");
        } else if (c == "4") rm.reportCard(ask("  Roll No: "));
        else if (c == "5") rm.summary();
        else if (c == "0") { cout << "  Data saved. Goodbye!\n"; break; }
        else cout << "  Invalid choice.\n";
        if (!cin) break;
    }
    cout << "\n  Press Enter to close...";
    string pause;
    getline(cin, pause);
    return 0;
}
