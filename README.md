# Library & Resource Management System (C++)

A console-based Library and Resource Management System built in C++ as part of the **Object Oriented Programming (CS-116)** term project at NED University of Engineering and Technology.

## Overview

Libraries handling large inventories of books and journals, alongside member accounts, borrowing records, reservations, and fines, run into errors and inconsistency when managed manually or through fragmented systems. This project automates the full workflow for two roles:

- **Member** — register, log in, browse/search the catalog, borrow and return resources, place/cancel reservations, view history, and pay overdue fines from a prepaid balance.
- **Administrator** — manage the resource catalog (add/update/remove), generate member and overdue reports, suspend/reactivate accounts, and update admin credentials.

All data persists across sessions using five CSV files (`resources.txt`, `users.txt`, `borrowrecords.txt`, `reservations.txt`, `fines.txt`) — no database required. Overdue fines (PKR 5/day) are calculated automatically, and three membership tiers (Basic, Standard, Premium) govern borrow limits and loan duration.

## Features

- Three-tier membership system with automatically enforced borrow limits and loan durations
- Reservation queue (up to 3 pending per member), auto-fulfilled when a copy is returned
- Automatic fine calculation and payment from a prepaid balance
- Full admin reporting suite: all members with borrow history, currently issued/overdue resources, all fines
- Case-insensitive search by title, author, or category
- Robust input validation, including leap-year-aware date checking (DD/MM/YYYY)
- Admin credentials stored separately and changeable at runtime

## OOP Concepts Demonstrated

| Concept | Where |
|---|---|
| Inheritance | `Book` and `Journal` inherit from abstract base class `Resource` |
| Polymorphism | `Resource*` pointers call `display()`, `getType()`, `toFileString()` dynamically at runtime |
| Encapsulation | Private data members (e.g. `User`'s balance/password) exposed only via controlled getters/setters |
| Abstraction | `Resource` is a pure abstract class with three pure virtual functions |
| Composition | `User` contains a `Membership` object by value |
| Aggregation | `Catalog` holds and manages the lifetime of `Resource*` pointers |
| Operator Overloading | `operator<<` for `Resource`; `operator>>` for `User`, `BorrowRecord`, `Fine`, `Reservation` |
| Exception Handling | Five custom exceptions derived from `std::runtime_error` |
| Templates | `loadList<T>()` / `saveList<T>()` generic file persistence |
| File I/O | `ifstream`/`ofstream` with custom CSV serialization |
| Static Members | `BorrowRecord::nextId`, `Reservation::nextId` for globally unique IDs |
| Friend Functions | `operator>>` declared as friend for direct private-member access |

## Tech Stack

- **Language:** C++
- **Standard library:** `<vector>`, `<fstream>`, `<sstream>`, `<stdexcept>`, `<ctime>`

## Sample Usage

```
================================================
  LIBRARY & RESOURCE MANAGEMENT SYSTEM
================================================
  [1]  Member Login
  [2]  Create New Account
  [3]  Admin Login
  [0]  Exit

  Choice: 1

  MEMBER LOGIN
  Username: ali
  Password: ali123
  Welcome, Ali Hassan!
```

## What We Learned

Working on this project introduced several C++ features not previously covered in depth:
- **`std::stringstream`** for robust input parsing and CSV token splitting
- **`std::vector`** for all dynamic collections, including passing by reference to avoid costly copies
- **File streams** — the difference between `ifstream`'s silent failure on a missing file vs. `ofstream`'s auto-creation, and using `getline()` for fields containing spaces
- **Function templates** — writing `loadList<T>()`/`saveList<T>()` once instead of per-class load/save logic, and the implicit contracts (`operator>>`, `toFileString()`) a type must satisfy to be used generically

The most challenging part was building a generic file-persistence layer that worked across all data classes without duplicated code, plus getting `Date` arithmetic right from scratch using `mktime()`/`difftime()` (leap years, month boundaries, and invalid `mktime()` results all needed guards).

## Team

Group project for CS-116, Spring 2026:
- Shifa Habib — Resource hierarchy (`Resource`, `Book`, `Journal`) and `Catalog`, including file handling and validation
- Fatima Zehra — `User`, `Membership`, `Fine` classes, authentication, and fine-calculation pipeline
- **Aliyya Afaq** — `BorrowRecord`, `Reservation`, and core `LibraryManager` logic (borrowing, returning, reservations, fine payments), plus integration and edge-case handling
- Maryam Imran — Console UI/menus, admin features, report generators, custom exceptions, and final integration

## Future Enhancements

- GUI interface (Qt / wxWidgets)
- Priority-based reservation handling by membership tier
- Membership upgrades through donations
- Damage fee tracking on return
- Automatic membership tier promotion based on borrow history
- Web interface via a REST API
- SQLite (or similar) database backend in place of flat-file CSV storage
