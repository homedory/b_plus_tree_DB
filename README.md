# B+ Tree Database

This project is a simple database implementation that uses a B+ Tree as the indexing structure. The database supports basic operations such as inserting, deleting, finding, and updating records.

## Features

- **Indexing**: Utilizes a B+ Tree for indexing data to ensure efficient lookups, insertions, and deletions.
  - **Key-Rotation Insertions**: This B+ Tree implementation employs key rotation during insertions to balance nodes efficiently and maintain the tree's optimal structure.
- **Data Model**:
  - **Key**: Integer (unique, no duplicates allowed)
  - **Value**: String
- **Operations**:
  - **Insert** (`i`): Add a new key-value pair.
  - **Delete** (`d`): Remove an existing key and its associated value.
  - **Find** (`f`): Retrieve the value associated with a given key.
  - **Change Table** (`c`): Switch between tables or initialize a new one.
  - **Join Tables** (`j`): Perform a natural join of two tables using the key.
  - **Quit Program** (`q`): Stop the application.

## Usage Instructions

### Running the Application

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```
2. Compile the project (if applicable):
   ```bash
   make
   ```
3. Run the application:
   ```bash
   ./main
   ```

4. (Optional) Compile the test case generator:
   ```bash
   cd tests
   gcc testcase_generator.c -o testcase_generator
   cd ..   

5. (Optional) Run the test case generator:
   ```bash
   ./tests/testcase_generator
   ```
   
### Commands

- **Insert** (`i`):
  Add a new record with a unique key.
  ```
  i <key> <value>
  Example: i 123 Hello
  ```

- **Delete** (`d`):
  Remove a record by its key.
  ```
  d <key>
  Example: d 123
  ```

- **Find** (`f`):
  Retrieve the value associated with a key.
  ```
  f <key>
  Example: f 123
  ```

- **Change Table** (`c`):
  Switch to another table between 1 and 2
  ```
  c <table_number>
  Example: c 1  # Switch to table1
           c 2  # Switch to table2
  ```

  - **Join Tables** (`j`):
  Perform a natural join of two tables based on their keys.
  ```
  j 
  Example: j 
  ```

- **Quit Program** (`q`):
  Stop the application.
  ```
  q
  ```


### Notes

- Keys must be unique integers.
- Values are strings.
- The B+ Tree ensures sorted order of keys and efficient operations.

## Page Layout

![page_layout](https://github.com/user-attachments/assets/7d15b6a5-8b7d-42d6-82c5-3e721e6aa57c)


## Wiki & TroubleShooting
[B+Tree Implementation][Wiki3]

[Wiki3]: https://soft-quince-6af.notion.site/Assignment3-Wiki-14365520635a8052b921c4a24bee5f18

[Natural Join Implementation][Wiki4]

[Wiki4]: https://soft-quince-6af.notion.site/Assignment4-Wiki-16065520635a806d92d2e9d7ee058b74




