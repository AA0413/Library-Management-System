/*
 * ============================================================
 *  LIBRARY AND RESOURCE MANAGEMENT SYSTEM
 *  Course: CS-116 Object Oriented Programming
 * ============================================================
 *
 *  Purpose:
 *  This program simulates a console-based library system. It allows
 *  members to register, log in, borrow and return resources, reserve
 *  unavailable items, pay fines, and view account information.
 *
 *  OOP concepts demonstrated:
 *  - Inheritance: Book and Journal inherit from Resource
 *  - Polymorphism: virtual functions call the correct derived method
 *  - Encapsulation: each class stores its own related data
 *  - Composition: User contains a Membership object
 *  - Aggregation: Catalog stores Resource pointers
 *  - Exception handling: custom exceptions handle invalid actions
 *  - File persistence: data is saved and loaded using CSV text files
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <ctime>
using namespace std;

// ===================== CONSTANTS =====================
const double FINE_PER_DAY = 5.0;
const int MAX_BORROW_DAYS_BASIC = 14;
const int MAX_BORROW_DAYS_STANDARD = 21;
const int MAX_BORROW_DAYS_PREMIUM = 30;
const int MAX_RESERVATIONS = 3;
const int MAX_ACTIVE_BORROWS_BASIC = 3;
const int MAX_ACTIVE_BORROWS_STANDARD = 5;
const int MAX_ACTIVE_BORROWS_PREMIUM = 10;

// ===================== CUSTOM EXCEPTIONS =====================
// These exceptions represent common problems that can happen during normal use.
// They make the code easier to debug and allow the menus to show clear messages.

// Raised when a member tries to borrow more items than allowed by their membership.
class BorrowLimitException : public runtime_error
{
public:
    BorrowLimitException(int limit)
        : runtime_error("Borrow limit of " + to_string(limit) + " active borrows reached.") {}
};

// Raised when the user does not have enough balance to pay a fine.
class InsufficientBalanceException : public runtime_error
{
public:
    InsufficientBalanceException(double required, double available)
        : runtime_error("Insufficient balance. Required: PKR " + to_string(required) +
                        ", Available: PKR " + to_string(available)) {}
};

// Raised when a resource ID does not match any item in the catalog.
class ResourceNotFoundException : public runtime_error
{
public:
    ResourceNotFoundException(int id)
        : runtime_error("Resource with ID " + to_string(id) + " not found.") {}
};

// Raised when the requested user account cannot be found.
class UserNotFoundException : public runtime_error
{
public:
    UserNotFoundException()
        : runtime_error("User account not found.") {}
};

// Raised when a file cannot be opened for saving data.
class FileIOException : public runtime_error
{
public:
    FileIOException(const string &filename)
        : runtime_error("Failed to open file: " + filename) {}
};

// Raised when an admin tries to remove a resource that is still borrowed by a member.
class ResourceInUseException : public runtime_error
{
public:
    ResourceInUseException(int id)
        : runtime_error("Resource with ID " + to_string(id) +
                        " cannot be removed - it is currently borrowed by one or more members.") {}
};

// ===================== UTILITY FUNCTIONS =====================
// Helper functions for input handling and simple CSV parsing.
// Reads a full line, including spaces.
string getLineInput(const string &prompt)
{
    cout << prompt;
    string input;
    getline(cin, input);
    return input;
}

// Keeps asking until the user enters a valid integer.
int getIntInput(const string &prompt)
{
    while (true)
    {
        string input = getLineInput(prompt);
        stringstream ss(input);
        int value;
        if (ss >> value)
            return value;
        cout << "  Invalid number. Try again.\n";
    }
}

// Keeps asking until the user enters a valid decimal number.
double getDoubleInput(const string &prompt)
{
    while (true)
    {
        string input = getLineInput(prompt);
        stringstream ss(input);
        double value;
        if (ss >> value)
            return value;
        cout << "  Invalid amount. Try again.\n";
    }
}

// Splits one comma-separated line into fields.
vector<string> splitCsv(const string &line)
{
    vector<string> fields;
    string field;
    stringstream ss(line);
    while (getline(ss, field, ','))
        fields.push_back(field);
    return fields;
}

// ===================== DATE =====================
// Stores a calendar date and supports validation and simple arithmetic.
struct Date
{
    int day, month, year;

    bool isValid() const
    {
        if (year < 1900 || year > 9999)
            return false;
        if (month < 1 || month > 12)
            return false;
        if (day < 1)
            return false;

        int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (leap)
            dim[1] = 29;
        return day <= dim[month - 1];
    }

    static Date today()
    {
        time_t t = time(0);
        if (t == (time_t)(-1))
            return {0, 0, 0};
        tm *now = localtime(&t);
        if (!now)
            return {0, 0, 0};
        return {now->tm_mday, now->tm_mon + 1, now->tm_year + 1900};
    }

    string toString() const
    {
        if (!isValid())
            return "Invalid Date";
        ostringstream oss;
        oss << (day < 10 ? "0" : "") << day << "/"
            << (month < 10 ? "0" : "") << month << "/"
            << year;
        return oss.str();
    }

    Date addDays(int days) const
    {
        if (!isValid())
            return {0, 0, 0};
        tm t = {};
        t.tm_mday = day + days;
        t.tm_mon = month - 1;
        t.tm_year = year - 1900;
        if (mktime(&t) == (time_t)(-1))
            return {0, 0, 0};
        return {t.tm_mday, t.tm_mon + 1, t.tm_year + 1900};
    }

    int diffDays(Date other) const
    {
        if (!isValid() || !other.isValid())
            return 0;
        tm t1 = {};
        t1.tm_mday = other.day;
        t1.tm_mon = other.month - 1;
        t1.tm_year = other.year - 1900;

        tm t2 = {};
        t2.tm_mday = day;
        t2.tm_mon = month - 1;
        t2.tm_year = year - 1900;

        time_t time1 = mktime(&t1);
        time_t time2 = mktime(&t2);
        if (time1 == (time_t)(-1) || time2 == (time_t)(-1))
            return 0;
        return (int)(difftime(time2, time1) / 86400.0);
    }

    bool operator==(const Date &other) const
    {
        return day == other.day && month == other.month && year == other.year;
    }

    bool operator<(const Date &other) const
    {
        if (year != other.year)
            return year < other.year;
        if (month != other.month)
            return month < other.month;
        return day < other.day;
    }

    void inputDate()
    {
        while (true)
        {
            string input = getLineInput("Enter date (DD/MM/YYYY): ");
            stringstream ss(input);
            char s1, s2;
            if (ss >> day >> s1 >> month >> s2 >> year && s1 == '/' && s2 == '/' && isValid())
                break;
            cout << "  Invalid date. Try again.\n";
        }
    }
};

// ===================== MEMBERSHIP =====================
// Stores the rules for one membership tier.
// The tier controls how many items can be borrowed and for how long.
class Membership
{
private:
    string level;
    int maxBooks;
    int borrowDurationDays;

public:
    Membership() : level("Basic"), maxBooks(MAX_ACTIVE_BORROWS_BASIC), borrowDurationDays(MAX_BORROW_DAYS_BASIC) {}

    void setLevel(const string &lvl)
    {
        level = lvl;
        if (level == "Basic")
        {
            maxBooks = MAX_ACTIVE_BORROWS_BASIC;
            borrowDurationDays = MAX_BORROW_DAYS_BASIC;
        }
        else if (level == "Standard")
        {
            maxBooks = MAX_ACTIVE_BORROWS_STANDARD;
            borrowDurationDays = MAX_BORROW_DAYS_STANDARD;
        }
        else if (level == "Premium")
        {
            maxBooks = MAX_ACTIVE_BORROWS_PREMIUM;
            borrowDurationDays = MAX_BORROW_DAYS_PREMIUM;
        }
        else
        {
            level = "Basic";
            maxBooks = MAX_ACTIVE_BORROWS_BASIC;
            borrowDurationDays = MAX_BORROW_DAYS_BASIC;
        }
    }

    int getMaxBooks() const { return maxBooks; }
    int getBorrowDuration() const { return borrowDurationDays; }
    string getLevel() const { return level; }
};

// ===================== ABSTRACT BASE CLASS: RESOURCE =====================
// Shared parent class for every item that can appear in the catalog.
// Book and Journal both inherit from this class.
class Resource
{
public:
    int resourceId;
    string title, author, category;
    int totalCopies, availableCopies;

    Resource() : resourceId(0), totalCopies(0), availableCopies(0) {}

    virtual void display() const = 0;
    virtual string toFileString() const = 0;
    virtual string getType() const = 0;

    bool checkAvailability() const { return availableCopies > 0; }

    friend ostream &operator<<(ostream &out, const Resource &r)
    {
        out << "  [" << r.resourceId << "] " << r.title
            << " by " << r.author
            << " | " << r.category
            << " | Available: " << r.availableCopies << "/" << r.totalCopies;
        return out;
    }

    virtual ~Resource() {}
};

// ===================== BOOK =====================
// Concrete resource type for books.
class Book : public Resource
{
public:
    string isbn;

    Book() : Resource() {}
    /*Default constructor.
    It calls Resource()'s constructor first (via the initializer list)
    to properly initialize the base class before Book itself.*/

    void display() const override
    {
        /*Overrides the pure virtual display() from Resource. const means it won't modify any member variables.
        override tells the compiler to verify this actually overrides a base function.*/
        cout << "------------------------------\n";
        cout << "Type     : Book\n";
        cout << "ID       : " << resourceId << "\n";
        cout << "Title    : " << title << "\n";
        cout << "Author   : " << author << "\n";
        cout << "Category : " << category << "\n";
        cout << "ISBN     : " << (isbn.empty() ? "N/A" : isbn) << "\n";
        /*A ternary operator — if isbn is an empty string, print "N/A", otherwise print the actual ISBN.*/
        cout << "Total    : " << totalCopies << "\n";
        cout << "Available: " << availableCopies << "\n";
    }

    string getType() const override { return "Book"; }
    /*Overrides the base class's getType(). Simply returns the string "Book" to identify this object's type at runtime.*/

    string toFileString() const override
    {
        return "Book," + to_string(resourceId) + "," +
               title + "," + author + "," + category + "," +
               to_string(totalCopies) + "," +
               to_string(availableCopies) + "," + isbn;
    }
    /*Serializes the object into a comma-separated string for saving to a file. to_string()
    converts integers to strings so they can be concatenated. The result looks
    like: Book,101,C++ Primer,Lippman,Programming,5,3,978-0321714114*/

    static Book fromFields(const vector<string> &f)
    {
        /*A static factory method — called on the class itself, not an instance.
        Takes a vector of strings (fields parsed from a file line) and builds a Book object from them.
        Think of it like a form parser:

        You have a line from a file:
        "Book,101,C++ Primer,Lippman,Programming,5,3,978-0321714114"

        You split it by commas → you get a list of strings → you pass that list to fromFields() → it reads each piece and assembles a proper Book object for you.
        It's basically a "build me an object from this raw data" helper function.*/
        Book b;
        b.resourceId = stoi(f[1]);
        b.title = f[2];
        b.author = f[3];
        b.category = f[4];
        b.totalCopies = stoi(f[5]);
        b.availableCopies = stoi(f[6]);
        if (f.size() > 7)
            b.isbn = f[7];
        /*"Is there a slot 7 in this box? Yes → grab it. No → skip it, don't crash.".
        These checks protect the program from crashing when the file data is incomplete or missing the last fields.*/
        return b;
        /*stoi() converts a string to an integer. Each index maps to a position in the comma-separated file line.*/
    }
};

// ===================== JOURNAL =====================
// Concrete resource type for journals.
class Journal : public Resource
{
public:
    int issueNumber;
    string publisher;

    Journal() : Resource(), issueNumber(0) {}

    void display() const override
    {
        cout << "------------------------------\n";
        cout << "Type      : Journal\n";
        cout << "ID        : " << resourceId << "\n";
        cout << "Title     : " << title << "\n";
        cout << "Author    : " << author << "\n";
        cout << "Category  : " << category << "\n";
        cout << "Publisher : " << (publisher.empty() ? "N/A" : publisher) << "\n";
        cout << "Issue No. : " << issueNumber << "\n";
        cout << "Total     : " << totalCopies << "\n";
        cout << "Available : " << availableCopies << "\n";
    }

    string getType() const override { return "Journal"; }

    string toFileString() const override
    {
        return "Journal," + to_string(resourceId) + "," +
               title + "," + author + "," + category + "," +
               to_string(totalCopies) + "," +
               to_string(availableCopies) + "," +
               to_string(issueNumber) + "," + publisher;
    }

    static Journal fromFields(const vector<string> &f)
    {
        Journal j;
        j.resourceId = stoi(f[1]);
        j.title = f[2];
        j.author = f[3];
        j.category = f[4];
        j.totalCopies = stoi(f[5]);
        j.availableCopies = stoi(f[6]);
        if (f.size() > 7)
            j.issueNumber = stoi(f[7]);
        if (f.size() > 8)
            j.publisher = f[8];
        /*A safety check — only reads isbn if it exists in the data (defensive programming against malformed/old file formats).*/
        return j;
    }
};

// ===================== CATALOG =====================
// Holds all resources and provides display, search, load, and save features.
class Catalog
{
private:
    vector<Resource *> resources;
    /*A list that stores pointers to Resource objects. Pointers are used because Resource is abstract —
     you can't store it directly, but you can point to its children (Book, Journal).
    */

    string toLower(string s) const
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (s[i] >= 'A' && s[i] <= 'Z')
                s[i] = char(s[i] - 'A' + 'a');
        }
        return s;
    }
    /*A list that stores pointers to Resource objects. Pointers are used because Resource is abstract —
     you can't store it directly, but you can point to its children (Book, Journal).
*/

public:
    ~Catalog()
    {
        for (size_t i = 0; i < resources.size(); i++)
            delete resources[i];
    }
    /*When the Catalog object is destroyed, this frees memory for every resource pointer.
    Since we used new to create them, we must use delete to clean up — otherwise memory leaks.
*/

    void loadFromFile(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open())
            return;
        // Opens the file for reading. If it can't open (file doesn't exist), it just returns and does nothing.

        string line;
        while (getline(file, line))
        {
            if (line.empty())
                continue;
            // Reads the file one line at a time. Skips empty lines.
            try
            {
                vector<string> f = splitCsv(line);
                if (f.empty())
                    continue;
                if (f[0] == "Book")
                {
                    resources.push_back(new Book(Book::fromFields(f)));
                }
                else if (f[0] == "Journal")
                {
                    resources.push_back(new Journal(Journal::fromFields(f)));
                }
                /*Splits the line by commas into a list
            Checks the first word (f[0]) to know the type
            Creates a new Book or new Journal using fromFields() and adds it to the list
            new stores it on the heap so it survives beyond this function*/
            }
            catch (const exception &e)
            {
                cerr << "  Warning: skipping malformed resource line: " << e.what() << "\n";
            }
            /*If any line causes an error (bad data, wrong format), it skips that line and prints
             a warning instead of crashing the whole program.
*/
        }
    }

    void saveToFile(const string &filename) const
    {
        ofstream file(filename);
        if (!file.is_open())
            throw FileIOException(filename);
        for (size_t i = 0; i < resources.size(); i++)
            file << resources[i]->toFileString() << "\n";
    }
    /*Opens file for writing. If it fails, throws an exception (unlike loadFromFile which just returns).
    Then writes each resource as a comma-separated line using toFileString().
    */

    Resource *findById(int id) const
    {
        for (size_t i = 0; i < resources.size(); i++)
        {
            if (resources[i]->resourceId == id)
                return resources[i];
        }
        return nullptr;
    }
    /*Loops through all resources looking for a matching ID. Returns the pointer if found, or nullptr (nothing) if not found.*/

    void displayAll() const
    {
        if (resources.empty())
        {
            cout << "  No resources in catalog.\n";
            return;
        }
        for (size_t i = 0; i < resources.size(); i++)
            resources[i]->display();
    }
    /*If list is empty, says so and exits. Otherwise calls display() on every resource —
    polymorphism makes each one print its own format (Book or Journal).*/

    void displayAvailableOnly() const
    {
        bool found = false;
        for (size_t i = 0; i < resources.size(); i++)
        {
            if (resources[i]->checkAvailability())
            {
                resources[i]->display();
                found = true;
            }
        }
        if (!found)
            cout << "  No available resources right now.\n";
    }
    /*Same but only displays resources where checkAvailability() returns true (copies are available).
     Uses a found flag to detect if nothing was available.
    */

    void search(const string &keyword, int type) const
    {
        bool found = false;
        for (size_t i = 0; i < resources.size(); i++)
        {
            Resource *r = resources[i];
            string val = (type == 1) ? r->title : (type == 2) ? r->author
                                                              : r->category;
            if (toLower(val).find(toLower(keyword)) != string::npos)
            {
                r->display();
                found = true;
            }
        }
        if (!found)
            cout << "  No matching resource found.\n";
    }
    /*type decides what field to search (1=title, 2=author, else=category)
    Both values are lowercased so search is case-insensitive.
    find() looks for the keyword inside the field — string::npos means "not found"*/

    vector<Resource *> &getAllResources() { return resources; }
    // Without & → returns a copy (expensive, and changes won't affect the original)
    // With & → returns the real list directly (fast, and changes affect the original)

    int getMaxResourceId() const
    {
        int maxId = 0;
        for (size_t i = 0; i < resources.size(); i++)
        {
            if (resources[i]->resourceId > maxId)
                maxId = resources[i]->resourceId;
        }
        return maxId;
    }
    /*Loops through all resources and tracks the highest ID found.
    Used when adding a new resource so the new ID doesn't clash with existing ones.*/
};

// ===================== BORROW RECORD =====================
// Stores one borrowing transaction from issue date to return date.
class BorrowRecord
{
private:
    int recordId, userId, resourceId;
    Date issueDate, dueDate, returnDate;
    bool isReturned;
    static int nextId;
    /*Stores one borrowing transaction. static int nextId is shared across all objects —
     it keeps increasing so every record gets a unique ID. */

public:
    BorrowRecord()
        : recordId(0), userId(0), resourceId(0), issueDate({0, 0, 0}), dueDate({0, 0, 0}),
          returnDate({0, 0, 0}), isReturned(false) {}

    void issueBook(int uId, int rId, int borrowDays)
    {
        recordId = ++nextId;
        /*++nextId increments first, then assigns — so IDs start from 1, not 0.*/
        userId = uId;
        resourceId = rId;
        cout << "Enter issue date:\n";
        issueDate.inputDate();
        dueDate = issueDate.addDays(borrowDays);
        isReturned = false;
        returnDate = {0, 0, 0};
    }
    /*Takes today's date as input, calculates due date by adding borrow days, marks as not returned, and clears return date.*/

    void returnBook()
    {
        if (!isReturned)
        {
            cout << "Enter return date:\n";
            returnDate.inputDate();
            isReturned = true;
        }
        /*Only processes return if not already returned. Records the return date and flips isReturned to true.*/
    }

    int calculateDaysLate() const
    {
        if (!isReturned)
            return 0;
        int late = returnDate.diffDays(dueDate);
        return (late > 0) ? late : 0;
    }
    /*If not returned yet, no late days. Otherwise calculates difference between
    return date and due date. If negative (returned early), returns 0.*/

    int getRecordId() const { return recordId; }
    int getUserId() const { return userId; }
    int getResourceId() const { return resourceId; }
    bool getStatus() const { return isReturned; }
    Date getIssueDate() const { return issueDate; }
    Date getDueDate() const { return dueDate; }
    Date getReturnDate() const { return returnDate; }

    string toFileString() const
    {
        return to_string(recordId) + "," + to_string(userId) + "," +
               to_string(resourceId) + "," +
               to_string(issueDate.day) + "," + to_string(issueDate.month) + "," + to_string(issueDate.year) + "," +
               to_string(dueDate.day) + "," + to_string(dueDate.month) + "," + to_string(dueDate.year) + "," +
               to_string(returnDate.day) + "," + to_string(returnDate.month) + "," + to_string(returnDate.year) + "," +
               to_string(isReturned);
    }

    friend istream &operator>>(istream &in, BorrowRecord &r)
    {
        /*Overloads the >> operator so you can read a BorrowRecord directly from a file stream like file >> record.*/
        string line;
        if (!getline(in, line) || line.empty())
            return in;
        vector<string> f = splitCsv(line);
        if (f.size() < 13)
            return in;
        // Safety check — a valid record needs exactly 13 fields.
        r.recordId = stoi(f[0]);
        r.userId = stoi(f[1]);
        r.resourceId = stoi(f[2]);
        r.issueDate = {stoi(f[3]), stoi(f[4]), stoi(f[5])};
        // Uses brace initialization to fill the Date struct directly from 3 separate fields.
        r.dueDate = {stoi(f[6]), stoi(f[7]), stoi(f[8])};
        r.returnDate = {stoi(f[9]), stoi(f[10]), stoi(f[11])};
        r.isReturned = stoi(f[12]);
        if (r.recordId > nextId)
            nextId = r.recordId;
        return in;
    }
};
int BorrowRecord::nextId = 0;

// ===================== RESERVATION =====================
// Stores a reservation request for a resource that is currently unavailable.
class Reservation
{
private:
    int reservationId, userId, resourceId;
    string status;
    static int nextId;

public:
    Reservation() : reservationId(0), userId(0), resourceId(0), status("") {}

    void addReservation(int uId, int rId)
    {
        reservationId = ++nextId;
        userId = uId;
        resourceId = rId;
        status = "Pending";
    }

    void cancelReservation()
    {
        if (status == "Pending")
            status = "Cancelled";
    }
    void fulfillReservation()
    {
        if (status == "Pending")
            status = "Fulfilled";
    }
    // Only changes status if still "Pending" — can't cancel something already fulfilled or vice versa.

    int getReservationId() const { return reservationId; }
    int getUserId() const { return userId; }
    int getResourceId() const { return resourceId; }
    string getStatus() const { return status; }
    bool isPending() const { return status == "Pending"; }

    string toFileString() const
    {
        return to_string(reservationId) + "," + to_string(userId) + "," +
               to_string(resourceId) + "," + status;
    }

    friend istream &operator>>(istream &in, Reservation &r)
    {
        string line;
        if (!getline(in, line) || line.empty())
            return in;
        vector<string> f = splitCsv(line);
        if (f.size() < 4)
            return in;
        r.reservationId = stoi(f[0]);
        r.userId = stoi(f[1]);
        r.resourceId = stoi(f[2]);
        r.status = f[3];
        if (r.reservationId > nextId)
            nextId = r.reservationId;
        return in;
    }
};
int Reservation::nextId = 0;

// ===================== FINE =====================
// Stores the late fee charged for an overdue borrow record.
class Fine
{
private:
    int fineId, userId, recordId;
    double amount;
    bool isPaid;

public:
    Fine() : fineId(0), userId(0), recordId(0), amount(0.0), isPaid(false) {}
    Fine(int id, int uId, int rId) : fineId(id), userId(uId), recordId(rId), amount(0.0), isPaid(false) {}
    // A second constructor that takes IDs directly. Amount starts at 0 until calculated.

    void calculateFine(int daysLate) { amount = (daysLate > 0) ? daysLate * FINE_PER_DAY : 0.0; }
    // Multiplies late days by a constant FINE_PER_DAY (defined elsewhere). If not late, fine is 0.
    void markPaid() { isPaid = true; }
    // Simply marks the fine as paid.

    int getFineId() const { return fineId; }
    int getUserId() const { return userId; }
    int getRecordId() const { return recordId; }
    double getAmount() const { return amount; }
    bool getStatus() const { return isPaid; }

    string toFileString() const
    {
        return to_string(fineId) + "," + to_string(userId) + "," + to_string(recordId) + "," +
               to_string(amount) + "," + to_string(isPaid);
    }

    friend istream &operator>>(istream &in, Fine &f)
    {
        string line;
        if (!getline(in, line) || line.empty())
            return in;
        vector<string> fld = splitCsv(line);
        if (fld.size() < 5)
            return in;
        f.fineId = stoi(fld[0]);
        f.userId = stoi(fld[1]);
        f.recordId = stoi(fld[2]);
        f.amount = stod(fld[3]);
        // stod() converts string to a double (decimal number) — used here because fine amount can have cents.
        f.isPaid = stoi(fld[4]);
        return in;
    }
};

// ===================== USER =====================
// Stores one library member account, including login details and balance.
class User
{
private:
    int userId;
    string username, password, name;
    double balance;
    bool isActive;
    Membership membership;

public:
    User() : userId(0), balance(0.0), isActive(false) {}
    // Creates an empty user with default values. Used when loading from file (fills values later).

    User(int id, const string &uname, const string &pass, const string &fullName,
         const string &membershipLevel = "Basic")
        : userId(id), username(uname), password(pass), name(fullName), balance(0.0), isActive(true)
    {
        membership.setLevel(membershipLevel);
    }
    /*Creates a real user with actual data.

membershipLevel = "Basic" → default value, so if you don't pass it, it automatically becomes "Basic"
balance(0.0) → new users start with zero balance
isActive(true) → new users are active by default
membership.setLevel() → sets the membership level separately since it's an object, not a simple type*/

    bool login(const string &uname, const string &pass) const
    {
        return isActive && username == uname && password == pass;
    }
    /*Returns true only if all three conditions are met:

    Account is active
    Username matches
    Password matches

    If any one fails, login fails.*/

    int getUserId() const { return userId; }
    string getUsername() const { return username; }
    string getName() const { return name; }
    double getBalance() const { return balance; }
    bool getIsActive() const { return isActive; }
    const Membership &getMembership() const { return membership; }
    Membership &getMembership() { return membership; }
    /*Two versions of the same getter:
    First one → for read-only access (when the User itself is const)
    Second one → for read and write access (when you need to modify membership)
    */

    void deactivate() { isActive = false; }
    void activate() { isActive = true; }
    // Simply flips the isActive flag. A deactivated user can't log in (blocked by login()).

    void deposit(double amount)
    {
        if (amount > 0)
            balance += amount;
    }
    // Adds money to balance. The if check prevents adding negative or zero amounts.

    void deductBalance(double amount)
    {
        if (amount > balance)
            throw InsufficientBalanceException(amount, balance);
        if (amount > 0)
            balance -= amount;
    }
    /*First checks if user has enough money — if not, throws an exception (stops the operation with an error)
    Then deducts the amount if it's positive
    Protects against the balance going negative
    */

    string toFileString() const
    {
        return to_string(userId) + "," + username + "," + password + "," + name + "," +
               to_string(balance) + "," + membership.getLevel() + "," + to_string(isActive);
    }
    // Converts user data into one comma-separated line for saving to file. Looks like:
    // 1,ali123,pass99,Ali Khan,250.0,Basic,1

    friend istream &operator>>(istream &in, User &u)
    {
        string line;
        if (!getline(in, line) || line.empty())
            return in;
        vector<string> f = splitCsv(line);
        if (f.size() < 6)
            return in;
        // Reads one line from file, splits by commas, and checks there are at least 6 fields.
        u.userId = stoi(f[0]);
        u.username = f[1];
        u.password = f[2];
        u.name = f[3];
        u.balance = stod(f[4]);
        u.membership.setLevel(f[5]);
        u.isActive = (f.size() > 6) ? stoi(f[6]) : 1;
        /*Fills each field from the split values. The last line is a safety check —
        if isActive field is missing in old files, it defaults to 1 (active).*/
        return in;
    }
};

// ===================== TEMPLATE HELPERS FOR FILE I/O =====================
// Generic load/save helpers used by multiple classes.
template <typename T>
/*T is a placeholder for any type. When you call the function, T gets replaced with the actual type like User, BorrowRecord, Fine, etc.
Think of it like a blank form — same structure, different data type each time.
*/
void loadList(vector<T> &items, const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
        return;
    T item;
    while (file >> item)
        items.push_back(item);
}
/*Opens the file
Creates a temporary empty object item
Reads one record at a time using >> (which we overloaded earlier)
Adds each loaded object to the list
Works for any type that has operator>> defined

So instead of writing separate load functions for User, BorrowRecord, Fine — you write one function that works for all of them.*/

template <typename T>
void saveList(const vector<T> &items, const string &filename)
{
    ofstream file(filename);
    if (!file.is_open())
        throw FileIOException(filename);
    for (size_t i = 0; i < items.size(); i++)
        file << items[i].toFileString() << "\n";
}
/*Opens file for writing
Throws exception if file can't open
Loops through every item and writes it as a line using toFileString()
Again works for any type that has toFileString() defined
*/

// ===================== LIBRARY MANAGER =====================
// This is the main controller class.
// It connects users, resources, borrowing, reservations, fines, and reports.
class LibraryManager
{
private:
    Catalog catalog;
    vector<User> users;
    vector<BorrowRecord> records;
    vector<Reservation> reservations;
    vector<Fine> fines;
    int nextFineId = 1;
    int nextUserId = 1;

public:
    void loadAll()
    {
        catalog.loadFromFile("resources.txt");
        loadList(users, "users.txt");
        loadList(records, "borrowrecords.txt");
        loadList(reservations, "reservations.txt");
        loadList(fines, "fines.txt");
        // Loads everything from their respective files when the program starts.

        for (size_t i = 0; i < fines.size(); i++)
        {
            if (fines[i].getFineId() >= nextFineId)
                nextFineId = fines[i].getFineId() + 1;
        }
        for (size_t i = 0; i < users.size(); i++)
        {
            if (users[i].getUserId() >= nextUserId)
                nextUserId = users[i].getUserId() + 1;
        }
        /*After loading, finds the highest existing ID and sets nextId one above it — so new records don't reuse old IDs.*/
    }

    void saveAll()
    {
        catalog.saveToFile("resources.txt");
        saveList(users, "users.txt");
        saveList(records, "borrowrecords.txt");
        saveList(reservations, "reservations.txt");
        saveList(fines, "fines.txt");
    } // Saves everything back to files when the program exits.

    User *findUserById(int id)
    {
        for (size_t i = 0; i < users.size(); i++)
        {
            if (users[i].getUserId() == id)
                return &users[i];
        }
        return nullptr;
    }

    User *findUserByUsername(const string &uname)
    {
        for (size_t i = 0; i < users.size(); i++)
        {
            if (users[i].getUsername() == uname)
                return &users[i];
        }
        return nullptr;
    }

    Resource *findResourceById(int id)
    {
        return catalog.findById(id);
    }
    /*All follow the same pattern — loop through the list, find a match, return a pointer to it (or nullptr if not found).*/

    BorrowRecord *findActiveRecord(int userId, int resourceId)
    {
        for (size_t i = 0; i < records.size(); i++)
        {
            if (records[i].getUserId() == userId &&
                records[i].getResourceId() == resourceId &&
                !records[i].getStatus())
            {
                return &records[i];
            }
        }
        return nullptr;
        /*Finds a borrow record that matches both the user and resource, and is not yet returned (!getStatus()).
        Used when processing a return.*/
    }

    Fine *findFineById(int fineId)
    {
        for (size_t i = 0; i < fines.size(); i++)
        {
            if (fines[i].getFineId() == fineId)
                return &fines[i];
        }
        return nullptr;
    }

    int countActiveBorrows(int userId)
    {
        int count = 0;
        for (size_t i = 0; i < records.size(); i++)
        {
            if (records[i].getUserId() == userId && !records[i].getStatus())
                count++;
        }
        return count;
    }
    /*Counts how many books a user currently has borrowed (not returned). Used to enforce borrow limits.*/

    bool isResourceCurrentlyBorrowed(int resourceId)
    {
        for (size_t i = 0; i < records.size(); i++)
        {
            if (records[i].getResourceId() == resourceId && !records[i].getStatus())
                return true;
        }
        return false;
    }
    /*Checks whether any BorrowRecord for this resource is still active (not returned).
    Used to stop admins from deleting a resource that a member currently has out —
    otherwise the borrow record and any future return/fine logic would point to a
    resource that no longer exists in the catalog.*/

    bool hasPendingReservations(int resourceId)
    {
        for (size_t i = 0; i < reservations.size(); i++)
        {
            if (reservations[i].getResourceId() == resourceId && reservations[i].isPending())
                return true;
        }
        return false;
    }
    /*Checks whether anyone has a pending reservation on this resource. Removing a
    reserved resource would leave the reservation pointing at nothing.*/

    void createFine(int userId, int recordId, int daysLate)
    {
        if (daysLate <= 0)
            return;
        Fine f(nextFineId++, userId, recordId);
        f.calculateFine(daysLate);
        fines.push_back(f);
        cout << "  Fine of PKR " << f.getAmount()
             << " created (Fine ID: " << f.getFineId() << ").\n";
    }
    /*If not late, does nothing
    Creates a Fine object with a unique ID
    Calculates the amount based on days late
    Adds it to the fines list
    */

    bool registerUser(const string &uname, const string &pass, const string &name)
    {
        if (findUserByUsername(uname))
        {
            cout << "  Username already exists.\n";
            return false;
        }
        users.push_back(User(nextUserId++, uname, pass, name));
        cout << "  Account created! Your User ID is " << (nextUserId - 1) << ".\n";
        return true;
    }
    /*Checks if username is already taken
    If not, creates a new User with a unique ID and adds to list
    nextUserId - 1 because nextUserId++ already incremented it*/

    User *loginUser(const string &uname, const string &pass)
    {
        for (size_t i = 0; i < users.size(); i++)
        {
            if (users[i].login(uname, pass))
                return &users[i];
        }
        return nullptr;
    }

    bool borrowResource(int userId, int resourceId)
    {
        try
        {
            User *user = findUserById(userId);
            if (!user)
                throw UserNotFoundException();

            Resource *res = findResourceById(resourceId);
            if (!res)
                throw ResourceNotFoundException(resourceId);

            if (!res->checkAvailability())
            {
                cout << "  No copies available. Would you like to reserve? (y/n): ";
                string ch;
                getline(cin, ch);
                if (ch == "y" || ch == "Y")
                    addReservation(userId, resourceId);
                return false;
            }

            int limit = user->getMembership().getMaxBooks();
            if (countActiveBorrows(userId) >= limit)
                throw BorrowLimitException(limit);

            res->availableCopies--;
            BorrowRecord record;
            record.issueBook(userId, resourceId, user->getMembership().getBorrowDuration());
            records.push_back(record);
            cout << "  Book borrowed! Due date: " << records.back().getDueDate().toString() << "\n";
            return true;
        }
        catch (const BorrowLimitException &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
        catch (const ResourceNotFoundException &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
        catch (const UserNotFoundException &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
        return false;
    }

    bool returnResource(int userId, int resourceId)
    {
        BorrowRecord *record = findActiveRecord(userId, resourceId);
        if (!record)
        {
            cout << "  No active borrow found for this resource.\n";
            return false;
        }

        record->returnBook();

        Resource *res = findResourceById(resourceId);
        if (res)
            res->availableCopies++;

        int daysLate = record->calculateDaysLate();
        if (daysLate > 0)
        {
            cout << "  Returned " << daysLate << " day(s) late!\n";
            createFine(userId, record->getRecordId(), daysLate);
        }

        for (size_t i = 0; i < reservations.size(); i++)
        {
            if (reservations[i].getResourceId() == resourceId && reservations[i].isPending())
            {
                reservations[i].fulfillReservation();
                cout << "  Reservation fulfilled for User ID " << reservations[i].getUserId() << ".\n";
                break;
            }
        }

        cout << "  Resource returned successfully.\n";
        return true;
    }

    bool addReservation(int userId, int resourceId)
    {
        try
        {
            User *user = findUserById(userId);
            Resource *res = findResourceById(resourceId);
            if (!user)
                throw UserNotFoundException();
            if (!res)
                throw ResourceNotFoundException(resourceId);

            if (res->checkAvailability())
            {
                cout << "  This resource is available — borrow it directly!\n";
                return false;
            }

            int pending = 0;
            for (size_t i = 0; i < reservations.size(); i++)
            {
                if (reservations[i].getUserId() == userId && reservations[i].isPending())
                    pending++;
            }

            if (pending >= MAX_RESERVATIONS)
            {
                cout << "  You already have " << MAX_RESERVATIONS << " pending reservations.\n";
                return false;
            }

            Reservation r;
            r.addReservation(userId, resourceId);
            reservations.push_back(r);
            cout << "  Reservation placed (Reservation ID: " << reservations.back().getReservationId() << ").\n";
            return true;
        }
        catch (const exception &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
        return false;
    }

    bool cancelReservation(int userId, int reservationId)
    {
        for (size_t i = 0; i < reservations.size(); i++)
        {
            if (reservations[i].getReservationId() == reservationId &&
                reservations[i].getUserId() == userId)
            {
                if (!reservations[i].isPending())
                {
                    cout << "  Reservation is not pending.\n";
                    return false;
                }
                reservations[i].cancelReservation();
                cout << "  Reservation cancelled.\n";
                return true;
            }
        }
        cout << "  Reservation not found.\n";
        return false;
    }

    bool payFine(int userId, int fineId)
    {
        try
        {
            User *user = findUserById(userId);
            Fine *f = findFineById(fineId);
            if (!user)
                throw UserNotFoundException();
            if (!f)
            {
                cout << "  Fine not found.\n";
                return false;
            }
            if (f->getUserId() != userId)
            {
                cout << "  This fine does not belong to you.\n";
                return false;
            }
            if (f->getStatus())
            {
                cout << "  Fine already paid.\n";
                return false;
            }

            user->deductBalance(f->getAmount());
            f->markPaid();
            cout << "  Fine of PKR " << f->getAmount() << " paid. New balance: PKR "
                 << user->getBalance() << ".\n";
            return true;
        }
        catch (const InsufficientBalanceException &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
        catch (const UserNotFoundException &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
        return false;
    }

    void showBorrowHistory(int userId)
    {
        bool found = false;
        cout << "\n===== Borrow History for User ID " << userId << " =====\n";
        for (size_t i = 0; i < records.size(); i++)
        {
            if (records[i].getUserId() == userId)
            {
                Resource *res = findResourceById(records[i].getResourceId());
                cout << "Record ID : " << records[i].getRecordId() << "\n";
                cout << "Resource  : " << (res ? res->title : "Unknown") << "\n";
                cout << "Type      : " << (res ? res->getType() : "N/A") << "\n";
                cout << "Issued    : " << records[i].getIssueDate().toString() << "\n";
                cout << "Due       : " << records[i].getDueDate().toString() << "\n";
                cout << "Returned  : " << (records[i].getStatus() ? records[i].getReturnDate().toString() : "Not yet") << "\n";
                cout << "---\n";
                found = true;
            }
        }
        if (!found)
            cout << "  No borrow history found.\n";
    }

    void showMyFines(int userId)
    {
        bool found = false;
        cout << "\n===== Fines for User ID " << userId << " =====\n";
        for (size_t i = 0; i < fines.size(); i++)
        {
            if (fines[i].getUserId() == userId)
            {
                cout << "Fine ID : " << fines[i].getFineId()
                     << " | Amount: PKR " << fines[i].getAmount()
                     << " | Status: " << (fines[i].getStatus() ? "Paid" : "Unpaid") << "\n";
                found = true;
            }
        }
        if (!found)
            cout << "  No fines found.\n";
    }

    void showMyReservations(int userId)
    {
        bool found = false;
        cout << "\n===== Reservations for User ID " << userId << " =====\n";
        for (size_t i = 0; i < reservations.size(); i++)
        {
            if (reservations[i].getUserId() == userId)
            {
                Resource *res = findResourceById(reservations[i].getResourceId());
                cout << "Reservation ID : " << reservations[i].getReservationId() << "\n";
                cout << "Resource       : " << (res ? res->title : "Unknown") << "\n";
                cout << "Status         : " << reservations[i].getStatus() << "\n";
                cout << "---\n";
                found = true;
            }
        }
        if (!found)
            cout << "  No reservations found.\n";
    }

    void reportAllMembers()
    {
        cout << "\n========== ALL MEMBERS REPORT ==========\n";
        if (users.empty())
        {
            cout << "  No users registered.\n";
            return;
        }
        for (size_t i = 0; i < users.size(); i++)
        {
            cout << "\nUser ID  : " << users[i].getUserId() << "\n";
            cout << "Name     : " << users[i].getName() << "\n";
            cout << "Username : " << users[i].getUsername() << "\n";
            cout << "Balance  : PKR " << users[i].getBalance() << "\n";
            cout << "Status   : " << (users[i].getIsActive() ? "Active" : "Suspended") << "\n";
            cout << "Membership: " << users[i].getMembership().getLevel() << "\n";
            cout << "Active Borrows: " << countActiveBorrows(users[i].getUserId()) << "\n";
            showBorrowHistory(users[i].getUserId());
        }
    }

    void reportIssuedOrOverdue()
    {
        cout << "\n========== ISSUED / OVERDUE RESOURCES ==========\n";
        Date now = Date::today();
        bool found = false;
        for (size_t i = 0; i < records.size(); i++)
        {
            if (!records[i].getStatus())
            {
                Resource *res = findResourceById(records[i].getResourceId());
                User *usr = findUserById(records[i].getUserId());
                cout << "\nRecord ID  : " << records[i].getRecordId() << "\n";
                cout << "Resource   : " << (res ? res->title : "Unknown") << "\n";
                cout << "Type       : " << (res ? res->getType() : "N/A") << "\n";
                cout << "Borrowed by: " << (usr ? usr->getName() : "Unknown")
                     << " (ID: " << records[i].getUserId() << ")\n";
                cout << "Issued     : " << records[i].getIssueDate().toString() << "\n";
                cout << "Due        : " << records[i].getDueDate().toString() << "\n";
                int daysLate = now.diffDays(records[i].getDueDate());
                if (daysLate > 0)
                    cout << "** OVERDUE by " << daysLate << " day(s) **\n";
                else
                    cout << "Status     : On time\n";
                found = true;
            }
        }
        if (!found)
            cout << "  No resources currently issued.\n";
    }

    void reportAllFines()
    {
        cout << "\n========== ALL FINES REPORT ==========\n";
        if (fines.empty())
        {
            cout << "  No fines recorded.\n";
            return;
        }
        for (size_t i = 0; i < fines.size(); i++)
        {
            User *usr = findUserById(fines[i].getUserId());
            BorrowRecord *rec = nullptr;
            for (size_t j = 0; j < records.size(); j++)
            {
                if (records[j].getRecordId() == fines[i].getRecordId())
                {
                    rec = &records[j];
                    break;
                }
            }
            Resource *res = rec ? findResourceById(rec->getResourceId()) : nullptr;
            cout << "\nFine ID     : " << fines[i].getFineId() << "\n";
            cout << "User        : " << (usr ? usr->getName() : "Unknown")
                 << " (ID: " << fines[i].getUserId() << ")\n";
            cout << "Record ID   : " << fines[i].getRecordId() << "\n";
            cout << "Resource    : " << (res ? res->title : "Unknown") << "\n";
            cout << "Amount      : PKR " << fines[i].getAmount() << "\n";
            cout << "Status      : " << (fines[i].getStatus() ? "Paid" : "Unpaid") << "\n";
        }
    }

    void adminAddResource()
    {
        cout << "\n  Resource type:\n  1. Book\n  2. Journal\n";
        int typeChoice = getIntInput("  Choice: ");

        int newId = catalog.getMaxResourceId() + 1;
        string title, author, category;

        cout << "  Title    : ";
        getline(cin, title);
        cout << "  Author   : ";
        getline(cin, author);
        cout << "  Category : ";
        getline(cin, category);
        int copies = getIntInput("  Copies   : ");

        if (typeChoice == 1)
        {
            Book *b = new Book();
            b->resourceId = newId;
            b->title = title;
            b->author = author;
            b->category = category;
            b->totalCopies = copies;
            b->availableCopies = copies;
            cout << "  ISBN (optional, press Enter to skip): ";
            getline(cin, b->isbn);
            catalog.getAllResources().push_back(b);
        }
        else
        {
            Journal *j = new Journal();
            j->resourceId = newId;
            j->title = title;
            j->author = author;
            j->category = category;
            j->totalCopies = copies;
            j->availableCopies = copies;
            j->issueNumber = getIntInput("  Issue Number: ");
            cout << "  Publisher (optional): ";
            getline(cin, j->publisher);
            catalog.getAllResources().push_back(j);
        }

        cout << "  Resource added with ID " << newId << ".\n";
    }

    void adminUpdateResource()
    {
        int id = getIntInput("  Enter Resource ID: ");
        Resource *r = findResourceById(id);
        if (!r)
        {
            cout << "  Resource not found.\n";
            return;
        }

        cout << "\n  1. Title\n  2. Author\n  3. Category\n  4. Total Copies\n";
        int choice = getIntInput("  Choice: ");
        switch (choice)
        {
        case 1:
            cout << "  New Title   : ";
            getline(cin, r->title);
            break;
        case 2:
            cout << "  New Author  : ";
            getline(cin, r->author);
            break;
        case 3:
            cout << "  New Category: ";
            getline(cin, r->category);
            break;
        case 4:
            r->totalCopies = getIntInput("  New Total Copies: ");
            if (r->availableCopies > r->totalCopies)
                r->availableCopies = r->totalCopies;
            break;
        default:
            cout << "  Invalid choice.\n";
            return;
        }
        cout << "  Resource updated.\n";
    }

    void adminRemoveResource()
    {
        int id = getIntInput("  Enter Resource ID: ");
        try
        {
            if (!findResourceById(id)) throw ResourceNotFoundException(id);
            if (isResourceCurrentlyBorrowed(id)) throw ResourceInUseException(id);

            if (hasPendingReservations(id))
            {
                cout << "  Note: this resource has pending reservation(s); they will be cancelled.\n";
                for (size_t i = 0; i < reservations.size(); i++)
                {
                    if (reservations[i].getResourceId() == id && reservations[i].isPending())
                        reservations[i].cancelReservation();
                }
            }

            vector<Resource *> &list = catalog.getAllResources();
            for (size_t i = 0; i < list.size(); i++)
            {
                if (list[i]->resourceId == id)
                {
                    delete list[i];
                    list.erase(list.begin() + i);
                    cout << "  Resource removed.\n";
                    return;
                }
            }
        }
        catch (const ResourceInUseException &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
        catch (const ResourceNotFoundException &e)
        {
            cout << "  Error: " << e.what() << "\n";
        }
    }

    void adminSetUserStatus(bool active)
    {
        int id = getIntInput("  Enter User ID: ");
        User *u = findUserById(id);
        if (!u)
        {
            cout << "  User not found.\n";
            return;
        }
        if (active)
            u->activate();
        else
            u->deactivate();
        cout << "  User " << u->getName() << (active ? " activated.\n" : " suspended.\n");
    }

    Catalog &getCatalog() { return catalog; }
};

// ===================== UI HELPERS =====================
void printHeader(const string &title)
{
    cout << "\n================================================\n";
    cout << "  " << title << "\n";
    cout << "================================================\n";
}

void pause()
{
    cout << "\n  Press Enter to continue...";
    string dummy;
    getline(cin, dummy);
}

// ===================== ADMIN CREDENTIALS =====================
// Admin credentials are kept separately so they can be changed easily.
string ADMIN_USERNAME = "admin";
string ADMIN_PASSWORD = "admin123";

void loadAdminCredentials()
{
    ifstream file("admin.txt");
    if (!file.is_open())
        return;
    string line;
    if (getline(file, line))
    {
        size_t comma = line.find(',');
        if (comma != string::npos)
        {
            ADMIN_USERNAME = line.substr(0, comma);
            ADMIN_PASSWORD = line.substr(comma + 1);
        }
    }
}

void saveAdminCredentials()
{
    ofstream file("admin.txt");
    if (!file.is_open())
    {
        cout << "  Warning: could not save admin credentials to admin.txt\n";
        return;
    }
    file << ADMIN_USERNAME << "," << ADMIN_PASSWORD << "\n";
}

// ===================== MEMBER MENU =====================
void memberMenu(LibraryManager &lib, User *user)
{
    int choice;
    do
    {
        printHeader("MEMBER MENU - " + user->getName());
        cout << "  [1]  View all resources\n";
        cout << "  [2]  View available resources only\n";
        cout << "  [3]  Search resources\n";
        cout << "  [4]  Borrow a resource\n";
        cout << "  [5]  Return a resource\n";
        cout << "  [6]  Reserve a resource\n";
        cout << "  [7]  Cancel a reservation\n";
        cout << "  [8]  View my reservations\n";
        cout << "  [9]  View my borrow history\n";
        cout << "  [10] View my fines\n";
        cout << "  [11] Pay a fine\n";
        cout << "  [12] Deposit balance\n";
        cout << "  [13] View my account\n";
        cout << "  [0]  Logout\n";
        choice = getIntInput("\n  Choice: ");

        switch (choice)
        {
        case 1:
            printHeader("ALL RESOURCES");
            lib.getCatalog().displayAll();
            pause();
            break;
        case 2:
            printHeader("AVAILABLE RESOURCES");
            lib.getCatalog().displayAvailableOnly();
            pause();
            break;
        case 3:
        {
            printHeader("SEARCH RESOURCES");
            cout << "  1. By Title\n  2. By Author\n  3. By Category\n";
            int sc = getIntInput("  Choice: ");
            string keyword;
            if (sc == 1)
                keyword = getLineInput("  Enter Title: ");
            else if (sc == 2)
                keyword = getLineInput("  Enter Author: ");
            else if (sc == 3)
                keyword = getLineInput("  Enter Category: ");
            else
            {
                cout << "  Invalid choice.\n";
                pause();
                break;
            }
            lib.getCatalog().search(keyword, sc);
            pause();
            break;
        }
        case 4:
        {
            printHeader("BORROW RESOURCE");
            lib.getCatalog().displayAvailableOnly();
            int rId = getIntInput("\n  Enter Resource ID to borrow: ");
            lib.borrowResource(user->getUserId(), rId);
            pause();
            break;
        }
        case 5:
        {
            printHeader("RETURN RESOURCE");
            lib.showBorrowHistory(user->getUserId());
            int rId = getIntInput("\n  Enter Resource ID to return: ");
            lib.returnResource(user->getUserId(), rId);
            pause();
            break;
        }
        case 6:
        {
            printHeader("RESERVE RESOURCE");
            lib.getCatalog().displayAll();
            int rId = getIntInput("\n  Enter Resource ID to reserve: ");
            lib.addReservation(user->getUserId(), rId);
            pause();
            break;
        }
        case 7:
        {
            printHeader("CANCEL RESERVATION");
            lib.showMyReservations(user->getUserId());
            int resId = getIntInput("\n  Enter Reservation ID to cancel: ");
            lib.cancelReservation(user->getUserId(), resId);
            pause();
            break;
        }
        case 8:
            lib.showMyReservations(user->getUserId());
            pause();
            break;
        case 9:
            lib.showBorrowHistory(user->getUserId());
            pause();
            break;
        case 10:
            lib.showMyFines(user->getUserId());
            pause();
            break;
        case 11:
        {
            printHeader("PAY FINE");
            lib.showMyFines(user->getUserId());
            int fId = getIntInput("\n  Enter Fine ID to pay: ");
            lib.payFine(user->getUserId(), fId);
            pause();
            break;
        }
        case 12:
        {
            printHeader("DEPOSIT BALANCE");
            cout << "  Current balance: PKR " << user->getBalance() << "\n";
            double amt = getDoubleInput("  Enter amount to deposit: PKR ");
            user->deposit(amt);
            cout << "  New balance: PKR " << user->getBalance() << "\n";
            pause();
            break;
        }
        case 13:
            printHeader("MY ACCOUNT");
            cout << "  User ID   : " << user->getUserId() << "\n";
            cout << "  Name      : " << user->getName() << "\n";
            cout << "  Username  : " << user->getUsername() << "\n";
            cout << "  Balance   : PKR " << user->getBalance() << "\n";
            cout << "  Membership: " << user->getMembership().getLevel() << "\n";
            cout << "  Status    : " << (user->getIsActive() ? "Active" : "Suspended") << "\n";
            pause();
            break;
        case 0:
            cout << "  Logged out.\n";
            break;
        default:
            cout << "  Invalid choice. Please try again.\n";
        }
    } while (choice != 0);
}

// ===================== ADMIN MENU =====================
void adminMenu(LibraryManager &lib)
{
    int choice;
    do
    {
        printHeader("ADMIN MENU");
        cout << "  [1]  View all resources\n";
        cout << "  [2]  Add a resource\n";
        cout << "  [3]  Update a resource\n";
        cout << "  [4]  Remove a resource\n";
        cout << "  [5]  Report: all members & borrow history\n";
        cout << "  [6]  Report: currently issued / overdue resources\n";
        cout << "  [7]  Suspend a user account\n";
        cout << "  [8]  Activate a user account\n";
        cout << "  [9]  Change admin username / password\n";
        cout << "  [10] View all fines\n";
        cout << "  [0]  Logout\n";
        choice = getIntInput("\n  Choice: ");

        switch (choice)
        {
        case 1:
            printHeader("ALL RESOURCES");
            lib.getCatalog().displayAll();
            pause();
            break;
        case 2:
            printHeader("ADD RESOURCE");
            lib.adminAddResource();
            pause();
            break;
        case 3:
            printHeader("UPDATE RESOURCE");
            lib.adminUpdateResource();
            pause();
            break;
        case 4:
            printHeader("REMOVE RESOURCE");
            lib.adminRemoveResource();
            pause();
            break;
        case 5:
            lib.reportAllMembers();
            pause();
            break;
        case 6:
            lib.reportIssuedOrOverdue();
            pause();
            break;
        case 7:
            printHeader("SUSPEND USER");
            lib.adminSetUserStatus(false);
            pause();
            break;
        case 8:
            printHeader("ACTIVATE USER");
            lib.adminSetUserStatus(true);
            pause();
            break;
        case 9:
        {
            printHeader("CHANGE ADMIN CREDENTIALS");
            cout << "  Current username: " << ADMIN_USERNAME << "\n\n";
            cout << "  (Press Enter without typing to keep the current value)\n\n";
            string newUser = getLineInput("  New username : ");
            if (!newUser.empty())
                ADMIN_USERNAME = newUser;
            string newPass = getLineInput("  New password : ");
            if (!newPass.empty())
                ADMIN_PASSWORD = newPass;
            saveAdminCredentials();
            cout << "\n  Credentials updated and saved to admin.txt\n";
            cout << "  Username is now: " << ADMIN_USERNAME << "\n";
            pause();
            break;
        }
        case 10:
            printHeader("ALL FINES");
            lib.reportAllFines();
            pause();
            break;
        case 0:
            cout << "  Admin logged out.\n";
            break;
        default:
            cout << "  Invalid choice. Please try again.\n";
        }
    } while (choice != 0);
}

// ===================== MAIN =====================
int main()
{
    loadAdminCredentials();

    LibraryManager lib;
    try
    {
        lib.loadAll();
    }
    catch (const exception &e)
    {
        cerr << "  Warning during data load: " << e.what() << "\n";
    }

    int choice;
    do
    {
        printHeader("LIBRARY & RESOURCE MANAGEMENT SYSTEM");
        cout << "  [1]  Member Login\n";
        cout << "  [2]  Create New Account\n";
        cout << "  [3]  Admin Login\n";
        cout << "  [0]  Exit\n";
        choice = getIntInput("\n  Choice: ");

        switch (choice)
        {
        case 1:
        {
            printHeader("MEMBER LOGIN");
            string uname = getLineInput("  Username: ");
            string pass = getLineInput("  Password: ");
            User *user = lib.loginUser(uname, pass);
            if (user)
            {
                cout << "  Welcome, " << user->getName() << "!\n";
                memberMenu(lib, user);
            }
            else
            {
                cout << "  Invalid credentials or account is suspended.\n";
            }
            break;
        }
        case 2:
        {
            printHeader("CREATE ACCOUNT");
            string name = getLineInput("  Full Name : ");
            string uname = getLineInput("  Username  : ");
            string pass = getLineInput("  Password  : ");
            lib.registerUser(uname, pass, name);
            break;
        }
        case 3:
        {
            printHeader("ADMIN LOGIN");
            string uname = getLineInput("  Username: ");
            string pass = getLineInput("  Password: ");
            if (uname == ADMIN_USERNAME && pass == ADMIN_PASSWORD)
            {
                cout << "  Admin login successful.\n";
                adminMenu(lib);
            }
            else
            {
                cout << "  Invalid admin credentials.\n";
            }
            break;
        }
        case 0:
            cout << "\n  Saving all data...\n";
            try
            {
                lib.saveAll();
                cout << "  Data saved. Goodbye!\n";
            }
            catch (const FileIOException &e)
            {
                cerr << "  Save error: " << e.what() << "\n";
            }
            break;
        default:
            cout << "  Invalid choice. Please try again.\n";
        }
    } while (choice != 0);

    return 0;
}
