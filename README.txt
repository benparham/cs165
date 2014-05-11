Things that should work:
- Sorted, unsorted and btree column creation, removal, insert, select, fetch, load
- Aggregation functions (max, min, sum, etc.)
- Math functions (add, sub, mul, etc.)
- Loop join

Things that do not work:
- btree load with very large data sets. There are some issues with client->server data transfer when the data set is very large. If I'm lucky I'll work it out soon and be able to push the fix, but doubtful.
- Joins other than loop
- Assignment 4

Tricky stuff to look out for:
- My implementation requires using tables. You have to be "using" one to do anything to/with it's columns. I've rigged the client to default use the table "test", so you shouldn't have to worry about this, but I figured the heads up couldn't hurt.
- In order to make some of the tests given out that use load work, I have the client search in the scripts/p2 directory to find any .csv file you may want to load. Loading a csv from elsewhere (for instance scripts/misc) requires changing the #define DEFAULT_SCRIPT_FOLDER constant at the top of src/client/client.c

Usage:
- use tableName								// Set active table to tableName
- create table tableName					// Create a table with name tableName
- remove table tableName					// Remove table with name tableName
- print var varName 						// Print result of a select/fetch/join
- create(columnName, "columnType")			// Create column of type with name
- remove(columnName)						// Remove column with name
- var=select(columnName)					// Select all from column
- var=select(columnName,val)				// Select value from column
- var=select(columnName,lowVal,highVal)		// Select range of values from column
- insert(columnName,val)					// Insert value into column
- fetch(columnName,varName)					// Fetch values from column using var
- var=fetch(columnName,varName)				// Same as above but store result
- load("fileName.csv")						// Load column(s) from a csv
- print(columnName)							// For debugging, prints in server
- min(varName)								// Minimum
- max(varName)								// Maximum
- sum(varName)								// Summation
- avg(varName)								// Average
- count(varName)							// Number of entries
- add(varName1,varName2)					// Addition
- sub(varName1,varName2)					// Subtraction
- mul(varName1,varName2)					// Multiplication
- div(varName1,varName2)					// Division
- var1,var2=loopjoin(varName1,varName2)		// Join via nested loop
- exit										// Exit the client session
