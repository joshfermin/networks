# Distributed File System
Compiled using gcc

### Usage
To run:
```bash
make clean
make 

# start servers:
./dfs /DFS1 10001 &
./dfs /DFS2 10002 &
./dfs /DFS3 10003 &
./dfs /DFS4 10004 &

# start client:
./dfc client/dfc.conf
```

Client commands:
```bash
username> PUT <file>
username> LIST
username> GET <file>
```

* PUT uploads a file to the DFS.
* GET downloads the specified file from the DFS.
* LIST shows all the files in the current user's directory.