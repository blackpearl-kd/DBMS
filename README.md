# 📘 Database Management System in C++

This project is a minimal, self-contained **Database Management System (DBMS)** implemented in C++. It is designed to mirror the internal architecture of real-world databases, focusing on core components such as memory management, row storage, and command parsing.

## 🎯 Objective

The primary goal of this project is to demonstrate foundational DBMS concepts using low-level C++ constructs. It reflects a hands-on understanding of data layout, serialization, memory management, and runtime interaction.

## ✨ Key Features

* In-memory database structure with rows grouped into pages
* REPL (Read-Eval-Print Loop) for interactive user command input
* Command support for:

  * `insert` – Insert a new entry
  * `select` – Retrieve all stored entries
  * `.exit` – Terminate the session
* Manual memory management using raw pointers and serialization
* Modular code architecture for scalability

## 🛠 System Architecture

* **Row**: Represents a database entry with `id`, `username`, and `email`
* **Page**: Fixed-size (4096 bytes) memory blocks holding multiple rows
* **Table**: Array of pages, managing overall storage and indexing
* **Cursor**: Abstraction for sequential row traversal
* **REPL Loop**: Accepts and processes user input in a persistent session

## 🚀 Getting Started

### Clone the Repository

```bash
git clone https://github.com/your-username/dbms-cpp.git
cd dbms-cpp
```

### Compile the Source Code

```bash
g++ db.cpp -o db
```

### Run the Database CLI

```bash
./db
```

## 💡 Usage Examples

### Insert Data

```bash
db > insert 1 kishlay kishlay@example.com
Executed.
```

### Select Data

```bash
db > select
(1, kishlay, kishlay@example.com)
```

### Exit CLI

```bash
db > .exit
```

## 📂 File Structure

```
dbms-cpp/
├── db.cpp       # Main source file containing database logic
├── README.md    # Project documentation
```

## 📌 Potential Enhancements

* File-based persistence layer for long-term data storage
* B+ Tree indexing for optimized queries
* SQL-like WHERE clauses and query parsing
* Enhanced input validation and error handling
* Unit testing framework for improved maintainability

## 👨‍💻 Author

[**Kishlay Raj**](https://github.com/blackpearl-kd)
[**Srijan Kumar**](https://github.com/opsrijan)

---

> "A database isn't just about storing data — it's about organizing memory, logic, and access into something powerful and fast."
